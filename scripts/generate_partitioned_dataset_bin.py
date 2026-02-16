#!/usr/bin/env python3
from __future__ import annotations

import argparse
import array
import multiprocessing as mp
import os
import pathlib
import queue
import random
import shutil
import struct
import sys
import tempfile
import time
import zlib
from typing import Any, Dict, List, Tuple


DEFAULT_PROGRESS_UPDATES = 200
UINT32_MAX = (1 << 32) - 1
UINT32_DOMAIN_SIZE = UINT32_MAX + 1

# Binary format:
# - Header: magic/version/global params (v2 only).
# - Partition table v2:
#   (values_offset, values_byte_size,
#    truth_offset, truth_byte_size,
#    n, d, values_encoding, truth_encoding, reserved) repeated p times.
# - Data blocks:
#   - values: zlib-compressed uint32 little-endian IDs.
#   - truth: zlib-compressed bitset where bit t=1 iff value at position t is a new distinct.
MAGIC = b"SATPDBN2"
VERSION = 2
ENCODING_ZLIB_U32_LE = 1
ENCODING_ZLIB_BITSET_LE = 2
HEADER_FMT = "<8sIQQQQ"
PARTITION_ENTRY_FMT = "<QQQQQQIII"


def _validate_params(n: int, d: int, p: int) -> None:
    if n < 0 or d < 0:
        raise ValueError("n and d must be non-negative")
    if p <= 0:
        raise ValueError("p must be > 0")
    if d > n:
        raise ValueError("d must be <= n")
    if n > 0 and d == 0:
        raise ValueError("d=0 requires n=0")
    # Values are stored as uint32 in the current binary format.
    # n can be arbitrarily large (per-partition stream length), but the distinct
    # domain cannot exceed the uint32 key space.
    id_domain_size = min(n, UINT32_DOMAIN_SIZE)
    if d > id_domain_size:
        raise ValueError(
            f"d must be <= min(n, 2^32) because values are uint32 (got d={d}, domain={id_domain_size})"
        )


def _derive_partition_seed(seed: int, partition_index: int) -> int:
    return (seed * 0x9E3779B97F4A7C15 + partition_index * 0xBF58476D1CE4E5B9) & ((1 << 64) - 1)


def _format_seconds(seconds: float) -> str:
    seconds = max(0, int(seconds))
    hours = seconds // 3600
    minutes = (seconds % 3600) // 60
    secs = seconds % 60
    if hours:
        return f"{hours:d}:{minutes:02d}:{secs:02d}"
    return f"{minutes:02d}:{secs:02d}"


def _print_progress(done: int, total: int, start_time: float) -> None:
    if total == 0:
        return
    elapsed = time.time() - start_time
    rate = done / elapsed if elapsed > 0 else 0.0
    eta = (total - done) / rate if rate > 0 else 0.0
    percent = 100.0 * done / total
    msg = f"\r{percent:6.2f}% {done}/{total} | {rate:,.0f} elems/s | ETA {_format_seconds(eta)}"
    sys.stderr.write(msg)
    sys.stderr.flush()


def _drain_progress_queue(progress_queue: Any,
                          produced: int,
                          total: int,
                          start_time: float,
                          show_progress: bool) -> int:
    if progress_queue is None:
        return produced

    while True:
        try:
            inc = progress_queue.get_nowait()
        except queue.Empty:
            break
        produced += int(inc)
        if show_progress and total > 0:
            _print_progress(produced, total, start_time)
    return produced


def _u32_chunk_bytes(buffer_u32: array.array) -> bytes:
    if not buffer_u32:
        return b""
    if sys.byteorder != "little":
        tmp = array.array("I", buffer_u32)
        tmp.byteswap()
        return tmp.tobytes()
    return buffer_u32.tobytes()


def _write_partition_fragment(task: Tuple[int, int, int, int, int, str, Any, int]) -> Tuple[int, str, str]:
    (
        part_idx,
        n_per_partition,
        d_per_partition,
        id_domain_size,
        part_seed,
        tmp_dir,
        progress_queue,
        progress_batch,
    ) = task
    rng = random.Random(part_seed)
    values_path = pathlib.Path(tmp_dir) / f"part_{part_idx}.values.binfrag"
    truth_path = pathlib.Path(tmp_dir) / f"part_{part_idx}.truth.binfrag"

    distinct_ids = rng.sample(range(id_domain_size), d_per_partition) if d_per_partition > 0 else []
    forced_by_pos: Dict[int, int] = {}
    if d_per_partition > 0:
        mandatory_positions = rng.sample(range(n_per_partition), d_per_partition)
        rng.shuffle(distinct_ids)
        for i, pos in enumerate(mandatory_positions):
            forced_by_pos[pos] = distinct_ids[i]

    pending_progress = 0
    chunk = array.array("I")
    chunk_capacity = 1 << 20  # values per flush (~4 MiB)
    truth_byte = 0
    truth_bit_pos = 0
    truth_chunk = bytearray()
    truth_chunk_capacity = 1 << 20  # raw truth bytes per flush (~1 MiB)
    seen_ids: set[int] = set()

    with values_path.open("wb", buffering=1024 * 1024) as out_values:
        with truth_path.open("wb", buffering=512 * 1024) as out_truth:
            values_compressor = zlib.compressobj(level=6)
            truth_compressor = zlib.compressobj(level=6)
            for i in range(n_per_partition):
                forced = forced_by_pos.get(i)
                if forced is not None:
                    value = forced
                else:
                    value = distinct_ids[rng.randrange(d_per_partition)]

                is_new = value not in seen_ids
                if is_new:
                    seen_ids.add(value)

                chunk.append(value)
                if len(chunk) >= chunk_capacity:
                    raw_chunk = _u32_chunk_bytes(chunk)
                    compressed_chunk = values_compressor.compress(raw_chunk)
                    if compressed_chunk:
                        out_values.write(compressed_chunk)
                    del chunk[:]

                if is_new:
                    truth_byte |= (1 << truth_bit_pos)
                truth_bit_pos += 1
                if truth_bit_pos == 8:
                    truth_chunk.append(truth_byte)
                    truth_byte = 0
                    truth_bit_pos = 0
                    if len(truth_chunk) >= truth_chunk_capacity:
                        compressed_truth = truth_compressor.compress(bytes(truth_chunk))
                        if compressed_truth:
                            out_truth.write(compressed_truth)
                        truth_chunk.clear()

                if progress_queue is not None:
                    pending_progress += 1
                    if pending_progress >= progress_batch:
                        progress_queue.put(pending_progress)
                        pending_progress = 0

            if chunk:
                raw_chunk = _u32_chunk_bytes(chunk)
                compressed_chunk = values_compressor.compress(raw_chunk)
                if compressed_chunk:
                    out_values.write(compressed_chunk)
                del chunk[:]
            out_values.write(values_compressor.flush())

            if truth_bit_pos != 0:
                truth_chunk.append(truth_byte)
            if truth_chunk:
                compressed_truth = truth_compressor.compress(bytes(truth_chunk))
                if compressed_truth:
                    out_truth.write(compressed_truth)
                truth_chunk.clear()
            out_truth.write(truth_compressor.flush())

    if d_per_partition > 0 and len(seen_ids) != d_per_partition:
        raise RuntimeError(
            f"Partition {part_idx}: generated distinct={len(seen_ids)} expected={d_per_partition}"
        )

    if progress_queue is not None and pending_progress > 0:
        progress_queue.put(pending_progress)

    return part_idx, str(values_path), str(truth_path)


def generate_partitioned_dataset_bin(output_dir: pathlib.Path,
                                     n: int,
                                     d: int,
                                     p: int,
                                     seed: int,
                                     show_progress: bool = False,
                                     workers: int | None = None,
                                     progress_batch: int | None = None) -> pathlib.Path:
    _validate_params(n, d, p)

    output_dir.mkdir(parents=True, exist_ok=True)
    out_name = f"dataset_n_{n}_d_{d}_p_{p}_s_{seed}.bin"
    out_path = output_dir / out_name

    max_workers = os.cpu_count() or 1
    if workers is not None and workers <= 0:
        raise ValueError("workers must be > 0")
    requested_workers = max_workers if workers is None else workers
    workers_eff = max(1, min(requested_workers, p, max_workers))

    total_elements = n * p
    if progress_batch is None:
        progress_batch = max(1, total_elements // DEFAULT_PROGRESS_UPDATES)
    if progress_batch <= 0:
        raise ValueError("progress_batch must be > 0")

    produced = 0
    start = time.time()
    values_part_paths: Dict[int, str] = {}
    truth_part_paths: Dict[int, str] = {}

    with tempfile.TemporaryDirectory(prefix="satp_parts_bin_", dir=str(output_dir)) as tmp_dir:
        id_domain_size = min(n, UINT32_DOMAIN_SIZE)
        tasks: List[Tuple[int, int, int, int, int, str, Any, int]] = []
        for part_idx in range(p):
            tasks.append((
                part_idx,
                n,
                d,
                id_domain_size,
                _derive_partition_seed(seed, part_idx),
                tmp_dir,
                None,
                progress_batch,
            ))

        if workers_eff == 1:
            for task in tasks:
                part_idx, values_part_path, truth_part_path = _write_partition_fragment(task)
                values_part_paths[part_idx] = values_part_path
                truth_part_paths[part_idx] = truth_part_path
                produced += n
                if show_progress and total_elements > 0:
                    _print_progress(produced, total_elements, start)
        else:
            ctx = mp.get_context("spawn")
            with ctx.Manager() as manager:
                progress_queue = manager.Queue()
                tasks = [
                    (part_idx, n_part, d_part, id_dom, part_seed, tdir, progress_queue, pbatch)
                    for (part_idx, n_part, d_part, id_dom, part_seed, tdir, _, pbatch) in tasks
                ]

                with ctx.Pool(processes=workers_eff) as pool:
                    async_results = [pool.apply_async(_write_partition_fragment, args=(task,)) for task in tasks]

                    while True:
                        produced = _drain_progress_queue(progress_queue, produced, total_elements, start, show_progress)
                        done = sum(1 for r in async_results if r.ready())
                        if done == len(async_results):
                            break
                        time.sleep(0.05)

                    for async_result in async_results:
                        part_idx, values_part_path, truth_part_path = async_result.get()
                        values_part_paths[part_idx] = values_part_path
                        truth_part_paths[part_idx] = truth_part_path

                    produced = _drain_progress_queue(progress_queue, produced, total_elements, start, show_progress)

        values_part_sizes: List[int] = []
        truth_part_sizes: List[int] = []
        for part_idx in range(p):
            values_size = pathlib.Path(values_part_paths[part_idx]).stat().st_size
            truth_size = pathlib.Path(truth_part_paths[part_idx]).stat().st_size
            values_part_sizes.append(values_size)
            truth_part_sizes.append(truth_size)

        header_size = struct.calcsize(HEADER_FMT)
        entry_size = struct.calcsize(PARTITION_ENTRY_FMT)
        first_data_offset = header_size + p * entry_size

        entries: List[Tuple[int, int, int, int, int, int, int, int, int]] = []
        current_offset = first_data_offset
        for part_idx in range(p):
            values_size = values_part_sizes[part_idx]
            values_offset = current_offset
            current_offset += values_size

            truth_size = truth_part_sizes[part_idx]
            truth_offset = current_offset
            current_offset += truth_size

            entries.append((
                values_offset,
                values_size,
                truth_offset,
                truth_size,
                n,
                d,
                ENCODING_ZLIB_U32_LE,
                ENCODING_ZLIB_BITSET_LE,
                0,
            ))

        seed_u64 = seed & ((1 << 64) - 1)
        with out_path.open("wb", buffering=8 * 1024 * 1024) as out:
            out.write(struct.pack(HEADER_FMT, MAGIC, VERSION, n, d, p, seed_u64))
            for entry in entries:
                out.write(struct.pack(PARTITION_ENTRY_FMT, *entry))

            for part_idx in range(p):
                with pathlib.Path(values_part_paths[part_idx]).open("rb") as part_file:
                    shutil.copyfileobj(part_file, out, length=8 * 1024 * 1024)
                with pathlib.Path(truth_part_paths[part_idx]).open("rb") as part_file:
                    shutil.copyfileobj(part_file, out, length=8 * 1024 * 1024)

    if show_progress and total_elements > 0:
        if produced < total_elements:
            _print_progress(total_elements, total_elements, start)
        sys.stderr.write("\n")
        sys.stderr.flush()

    return out_path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate one compressed binary dataset with partitions and prefix truth F0(t) metadata (n,d,p,seed)."
    )
    parser.add_argument("--output-dir", type=pathlib.Path, default=pathlib.Path("."),
                        help="Directory where dataset_n_{n}_d_{d}_p_{p}_s_{seed}.bin is written")
    parser.add_argument("--n", required=True, type=int, help="Number of elements per partition")
    parser.add_argument("--d", required=True, type=int, help="Number of distinct elements per partition")
    parser.add_argument("--p", required=True, type=int, help="Number of partitions")
    parser.add_argument("--seed", required=True, type=int, help="Random seed")
    parser.add_argument("--workers", type=int, default=None,
                        help="Number of worker processes (default: CPU count)")
    parser.add_argument("--progress-batch", type=int, default=None,
                        help="Elements per worker progress update (default: auto ~= (n*p)/200)")
    parser.add_argument("--no-progress", action="store_true", help="Disable progress output")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    out_path = generate_partitioned_dataset_bin(
        output_dir=args.output_dir,
        n=args.n,
        d=args.d,
        p=args.p,
        seed=args.seed,
        show_progress=not args.no_progress,
        workers=args.workers,
        progress_batch=args.progress_batch,
    )
    print(out_path)


if __name__ == "__main__":
    main()
