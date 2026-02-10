#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import random
import sys
import time
from typing import List, Optional


def _validate_params(total: int, unique: int, max_value: Optional[int]) -> None:
    if total < 0 or unique < 0:
        raise ValueError("total and unique must be non-negative")
    if unique > total:
        raise ValueError("unique must be <= total")
    if unique == 0 and total > 0:
        raise ValueError("unique=0 requires total=0")
    if max_value is not None and max_value < unique - 1:
        raise ValueError("max_value must be >= unique-1")


class FrequencyBuckets:
    """Exact frequency table using hash buckets with separate chaining."""

    _MASK64 = (1 << 64) - 1

    def __init__(self, bucket_count: int) -> None:
        if bucket_count <= 0:
            raise ValueError("bucket_count must be > 0")
        # Each bucket stores [key, count] pairs.
        self._buckets: List[List[List[int]]] = [[] for _ in range(bucket_count)]
        self.distinct_count = 0
        self.total_updates = 0

    @classmethod
    def _mix64(cls, x: int) -> int:
        # splitmix64 finalizer: deterministic, fast, and stable across runs.
        z = (x + 0x9E3779B97F4A7C15) & cls._MASK64
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & cls._MASK64
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & cls._MASK64
        return z ^ (z >> 31)

    def _bucket_index(self, key: int) -> int:
        mixed = self._mix64(key & self._MASK64)
        return mixed % len(self._buckets)

    def increment(self, key: int) -> None:
        idx = self._bucket_index(key)
        chain = self._buckets[idx]
        for pair in chain:
            if pair[0] == key:
                pair[1] += 1
                self.total_updates += 1
                return
        chain.append([key, 1])
        self.distinct_count += 1
        self.total_updates += 1

    def get(self, key: int) -> int:
        idx = self._bucket_index(key)
        for pair in self._buckets[idx]:
            if pair[0] == key:
                return pair[1]
        return 0


def _format_seconds(seconds: float) -> str:
    seconds = max(0, int(seconds))
    hours = seconds // 3600
    minutes = (seconds % 3600) // 60
    secs = seconds % 60
    if hours:
        return f"{hours:d}:{minutes:02d}:{secs:02d}"
    return f"{minutes:02d}:{secs:02d}"


def _next_pow2(value: int) -> int:
    n = 1
    while n < value:
        n <<= 1
    return n


def _pick_unique_values(unique: int,
                        rng: random.Random,
                        max_value: Optional[int]) -> List[int]:
    if unique == 0:
        return []
    if max_value is None:
        values = list(range(unique))
        rng.shuffle(values)
        return values
    return rng.sample(range(max_value + 1), unique)


def generate_dataset(path: pathlib.Path,
                     total: int,
                     unique: int,
                     seed: Optional[int] = None,
                     max_value: Optional[int] = None,
                     show_progress: bool = False) -> None:
    _validate_params(total, unique, max_value)
    rng = random.Random(seed)
    unique_values = _pick_unique_values(unique, rng, max_value)

    # Keep buckets sparse enough while still exercising collision handling.
    bucket_count = _next_pow2(max(1, unique * 2))
    freq = FrequencyBuckets(bucket_count=bucket_count)

    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", buffering=1024 * 1024) as out:
        out.write(f"{total} {unique}\n")
        if total == 0:
            return

        update_every = max(1, total // 200)
        chunk_size = 100_000
        buffer: List[str] = []
        append_buffer = buffer.append
        start_time = time.time()
        produced = 0

        def emit(value: int) -> None:
            nonlocal produced
            freq.increment(value)
            append_buffer(str(value))
            produced += 1

            if len(buffer) >= chunk_size:
                out.write("\n".join(buffer))
                out.write("\n")
                buffer.clear()

            if show_progress and (produced % update_every == 0 or produced == total):
                elapsed = time.time() - start_time
                rate = produced / elapsed if elapsed > 0 else 0.0
                eta = (total - produced) / rate if rate > 0 else 0.0
                percent = 100.0 * produced / total
                msg = (f"\r{percent:6.2f}% {produced}/{total} "
                       f"| {rate:,.0f} elems/s | ETA {_format_seconds(eta)}")
                sys.stderr.write(msg)
                sys.stderr.flush()

        # Guarantee exact number of distinct elements: each unique appears once.
        for value in unique_values:
            emit(value)

        remaining = total - unique
        for _ in range(remaining):
            emit(unique_values[rng.randrange(unique)])

        if buffer:
            out.write("\n".join(buffer))
            out.write("\n")

        if show_progress:
            sys.stderr.write("\n")
            sys.stderr.flush()

    # Sanity checks: exactness for both total count and number of distincts.
    if freq.total_updates != total:
        raise RuntimeError("internal error: total updates mismatch")
    if freq.distinct_count != unique:
        raise RuntimeError("internal error: distinct count mismatch")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate a dataset file for SATP tests.")
    parser.add_argument("--output", required=True, type=pathlib.Path, help="Output dataset path")
    parser.add_argument("--total", required=True, type=int, help="Total number of elements")
    parser.add_argument("--unique", required=True, type=int, help="Number of unique elements")
    parser.add_argument("--seed", type=int, default=None, help="Random seed")
    parser.add_argument("--max-value", type=int, default=None, help="Max value for unique sampling")
    parser.add_argument("--no-progress", action="store_true", help="Disable progress output")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    generate_dataset(
        args.output,
        total=args.total,
        unique=args.unique,
        seed=args.seed,
        max_value=args.max_value,
        show_progress=not args.no_progress,
    )


if __name__ == "__main__":
    main()
