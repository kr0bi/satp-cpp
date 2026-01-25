#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest

SCRIPT_DIR = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

from generate_dataset import generate_dataset  # noqa: E402


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


if __name__ == "__main__":
    unittest.main()
