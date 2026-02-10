#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest

SCRIPT_DIR = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

from generate_dataset import generate_dataset  # noqa: E402
from generate_dataset_puppis import FrequencyBuckets, generate_dataset as generate_dataset_puppis  # noqa: E402


def read_dataset(path: pathlib.Path) -> tuple[int, int, list[int]]:
    with path.open("r", encoding="utf-8") as f:
        header = f.readline().split()
        if len(header) != 2:
            raise ValueError("Header must contain two integers")
        total, distinct = (int(header[0]), int(header[1]))
        values = [int(line.strip()) for line in f if line.strip()]
    return total, distinct, values


class GenerateDatasetTests(unittest.TestCase):
    def test_counts_and_header(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            path = pathlib.Path(tmpdir) / "dataset.txt"
            generate_dataset(path, total=1000, unique=100, seed=123)

            total, distinct, values = read_dataset(path)
            self.assertEqual(total, len(values))
            self.assertEqual(distinct, len(set(values)))
            self.assertTrue(all(0 <= v < distinct for v in values))

    def test_max_value_bounds(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            path = pathlib.Path(tmpdir) / "dataset.txt"
            generate_dataset(path, total=500, unique=50, seed=7, max_value=1000)

            total, distinct, values = read_dataset(path)
            self.assertEqual(total, len(values))
            self.assertEqual(distinct, len(set(values)))
            self.assertTrue(all(0 <= v <= 1000 for v in values))

    def test_invalid_unique(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            path = pathlib.Path(tmpdir) / "dataset.txt"
            with self.assertRaises(ValueError):
                generate_dataset(path, total=10, unique=20)

    def test_puppis_counts_and_header(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            path = pathlib.Path(tmpdir) / "dataset_puppis.txt"
            generate_dataset_puppis(path, total=1200, unique=120, seed=42, max_value=10_000)

            total, distinct, values = read_dataset(path)
            self.assertEqual(total, len(values))
            self.assertEqual(distinct, len(set(values)))
            self.assertTrue(all(0 <= v <= 10_000 for v in values))

    def test_puppis_collision_handling_is_exact(self) -> None:
        buckets = FrequencyBuckets(bucket_count=1)  # all keys collide in one bucket
        for value in [10, 20, 10, 30, 20, 10]:
            buckets.increment(value)

        self.assertEqual(buckets.distinct_count, 3)
        self.assertEqual(buckets.total_updates, 6)
        self.assertEqual(buckets.get(10), 3)
        self.assertEqual(buckets.get(20), 2)
        self.assertEqual(buckets.get(30), 1)
        self.assertEqual(buckets.get(99), 0)


if __name__ == "__main__":
    unittest.main()
