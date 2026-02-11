#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest
import json

SCRIPT_DIR = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

from generate_dataset import generate_dataset  # noqa: E402
from generate_dataset_puppis import FrequencyBuckets, generate_dataset as generate_dataset_puppis  # noqa: E402
from generate_partitioned_dataset import generate_partitioned_dataset  # noqa: E402


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

    def test_partitioned_json_schema_and_counts(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = pathlib.Path(tmpdir)
            out_path = generate_partitioned_dataset(
                output_dir=out_dir,
                n=30,
                d=7,
                p=4,
                seed=123,
                show_progress=False,
            )

            self.assertEqual(out_path.name, "dataset_n_30_d_7_p_4.json")
            self.assertTrue(out_path.exists())

            with out_path.open("r", encoding="utf-8") as f:
                payload = json.load(f)

            self.assertEqual(payload["nOfElements"], 30)
            self.assertEqual(payload["distinct"], 7)
            self.assertEqual(payload["seed"], 123)
            self.assertEqual(len(payload["partizioni"]), 4)

            all_ids: list[int] = []
            total_stream_items = 0
            for partition in payload["partizioni"]:
                local_counts: dict[int, int] = {}
                stream = partition["stream"]
                total_stream_items += len(stream)
                for item in stream:
                    value = int(item["id"])
                    freq = int(item["freq"])
                    local_counts[value] = local_counts.get(value, 0) + 1
                    self.assertEqual(freq, local_counts[value])
                    all_ids.append(value)

            self.assertEqual(total_stream_items, 30)
            self.assertEqual(len(set(all_ids)), 7)

    def test_partitioned_json_invalid_params(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = pathlib.Path(tmpdir)
            with self.assertRaises(ValueError):
                generate_partitioned_dataset(
                    output_dir=out_dir,
                    n=10,
                    d=11,
                    p=2,
                    seed=1,
                    show_progress=False,
                )


if __name__ == "__main__":
    unittest.main()
