#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import random
import sys
import time
from typing import Iterable, List, Optional


def _validate_params(total: int, unique: int, max_value: Optional[int]) -> None:
    if total < 0 or unique < 0:
        raise ValueError("total and unique must be non-negative")
    if unique > total:
        raise ValueError("unique must be <= total")
    if unique == 0 and total > 0:
        raise ValueError("unique=0 requires total=0")
    if max_value is not None and max_value < unique - 1:
        raise ValueError("max_value must be >= unique-1")


def generate_values(total: int,
                    unique: int,
                    rng: random.Random,
                    max_value: Optional[int] = None) -> List[int]:
    _validate_params(total, unique, max_value)

    if total == 0:
        return []

    if max_value is None:
        unique_values = list(range(unique))
    else:
        unique_values = rng.sample(range(max_value + 1), unique)

    values: List[int] = list(unique_values)
    remaining = total - unique
    if remaining > 0:
        values.extend(unique_values[rng.randrange(unique)] for _ in range(remaining))

    return values


def _format_seconds(seconds: float) -> str:
    seconds = max(0, int(seconds))
    hours = seconds // 3600
    minutes = (seconds % 3600) // 60
    secs = seconds % 60
    if hours:
        return f"{hours:d}:{minutes:02d}:{secs:02d}"
    return f"{minutes:02d}:{secs:02d}"


def write_dataset(path: pathlib.Path,
                  total: int,
                  unique: int,
                  values: Iterable[int],
                  show_progress: bool = False) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", buffering=1024 * 1024) as out:
        out.write(f"{total} {unique}\n")
        if total == 0:
            return

        update_every = max(1, total // 200)
        chunk_size = 100_000
        buffer: List[str] = []
        buffer_reserve = chunk_size + 1
        buffer_extend = buffer.append
        start_time = time.time()

        count = 0
        for value in values:
            buffer_extend(str(value))
            count += 1

            if len(buffer) >= buffer_reserve:
                out.write("\n".join(buffer))
                out.write("\n")
                buffer.clear()

            if show_progress and (count % update_every == 0 or count == total):
                elapsed = time.time() - start_time
                rate = count / elapsed if elapsed > 0 else 0.0
                eta = (total - count) / rate if rate > 0 else 0.0
                percent = 100.0 * count / total
                msg = (f"\r{percent:6.2f}% {count}/{total} "
                       f"| {rate:,.0f} elems/s | ETA {_format_seconds(eta)}")
                sys.stderr.write(msg)
                sys.stderr.flush()

        if buffer:
            out.write("\n".join(buffer))
            out.write("\n")

        if show_progress:
            sys.stderr.write("\n")
            sys.stderr.flush()


def generate_dataset(path: pathlib.Path,
                     total: int,
                     unique: int,
                     seed: Optional[int] = None,
                     max_value: Optional[int] = None,
                     show_progress: bool = False) -> None:
    rng = random.Random(seed)
    values = generate_values(total, unique, rng, max_value=max_value)
    write_dataset(path, total, unique, values, show_progress=show_progress)


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
