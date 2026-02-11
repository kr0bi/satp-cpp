#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import random
import sys
import time
from typing import Dict, Iterator, List


def _validate_params(n: int, d: int, p: int) -> None:
    if n < 0 or d < 0:
        raise ValueError("n and d must be non-negative")
    if p <= 0:
        raise ValueError("p must be > 0")
    if d > n:
        raise ValueError("d must be <= n")
    if n > 0 and d == 0:
        raise ValueError("d=0 requires n=0")


def _partition_sizes(n: int, p: int) -> List[int]:
    base = n // p
    rem = n % p
    return [base + (1 if i < rem else 0) for i in range(p)]


def _pick_distinct_ids(n: int, d: int, rng: random.Random) -> List[int]:
    if d == 0:
        return []
    return rng.sample(range(n), d)


def _generate_ids(n: int, distinct_ids: List[int], rng: random.Random) -> Iterator[int]:
    if n == 0:
        return
    if not distinct_ids:
        raise ValueError("distinct_ids cannot be empty when n > 0")

    mandatory_positions = rng.sample(range(n), len(distinct_ids))
    shuffled_ids = list(distinct_ids)
    rng.shuffle(shuffled_ids)
    mandatory_by_pos = {pos: shuffled_ids[i] for i, pos in enumerate(mandatory_positions)}

    d = len(shuffled_ids)
    for pos in range(n):
        forced = mandatory_by_pos.get(pos)
        if forced is not None:
            yield forced
        else:
            yield shuffled_ids[rng.randrange(d)]


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


def generate_partitioned_dataset(output_dir: pathlib.Path,
                                 n: int,
                                 d: int,
                                 p: int,
                                 seed: int,
                                 show_progress: bool = False) -> pathlib.Path:
    _validate_params(n, d, p)
    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / f"dataset_n_{n}_d_{d}_p_{p}.json"

    rng = random.Random(seed)
    distinct_ids = _pick_distinct_ids(n, d, rng)
    sizes = _partition_sizes(n, p)
    id_stream = _generate_ids(n, distinct_ids, rng)

    produced = 0
    update_every = max(1, n // 200) if n > 0 else 1
    start = time.time()

    with out_path.open("w", encoding="utf-8", buffering=1024 * 1024) as out:
        out.write("{\n")
        out.write(f"  \"nOfElements\": {n},\n")
        out.write(f"  \"distinct\": {d},\n")
        out.write(f"  \"seed\": {seed},\n")
        out.write("  \"partizioni\": [\n")

        for part_idx, part_size in enumerate(sizes):
            if part_idx > 0:
                out.write(",\n")
            out.write("    {\n")
            out.write("      \"stream\": [\n")

            local_freq: Dict[int, int] = {}
            for i in range(part_size):
                value = next(id_stream)
                freq = local_freq.get(value, 0) + 1
                local_freq[value] = freq

                if i > 0:
                    out.write(",\n")
                out.write("        ")
                json.dump({"id": value, "freq": freq}, out, separators=(", ", ": "))

                produced += 1
                if show_progress and (produced % update_every == 0 or produced == n):
                    _print_progress(produced, n, start)

            out.write("\n")
            out.write("      ]\n")
            out.write("    }")

        out.write("\n")
        out.write("  ]\n")
        out.write("}\n")

    if show_progress and n > 0:
        sys.stderr.write("\n")
        sys.stderr.flush()

    return out_path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate one JSON dataset with precomputed partitions (n, d, p, seed)."
    )
    parser.add_argument("--output-dir", type=pathlib.Path, default=pathlib.Path("."),
                        help="Directory where dataset_n_{n}_d_{d}_p_{p}.json is written")
    parser.add_argument("--n", required=True, type=int, help="Total number of elements")
    parser.add_argument("--d", required=True, type=int, help="Number of distinct elements")
    parser.add_argument("--p", required=True, type=int, help="Number of partitions")
    parser.add_argument("--seed", required=True, type=int, help="Random seed")
    parser.add_argument("--no-progress", action="store_true", help="Disable progress output")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    out_path = generate_partitioned_dataset(
        output_dir=args.output_dir,
        n=args.n,
        d=args.d,
        p=args.p,
        seed=args.seed,
        show_progress=not args.no_progress,
    )
    print(out_path)


if __name__ == "__main__":
    main()
