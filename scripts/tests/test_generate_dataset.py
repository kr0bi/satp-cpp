#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest
import json

SCRIPT_DIR = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

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
            self.assertEqual(payload["nOfPartitions"], 4)
            self.assertEqual(payload["totalElements"], 120)
            self.assertEqual(len(payload["partizioni"]), 4)

            total_stream_items = 0
            for partition in payload["partizioni"]:
                local_counts: dict[int, int] = {}
                stream = partition["stream"]
                total_stream_items += len(stream)
                self.assertEqual(len(stream), 30)
                for item in stream:
                    value = int(item["id"])
                    freq = int(item["freq"])
                    local_counts[value] = local_counts.get(value, 0) + 1
                    self.assertEqual(freq, local_counts[value])
                self.assertEqual(len(local_counts), 7)

            self.assertEqual(total_stream_items, 120)

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

    def test_partitioned_json_deterministic_across_workers(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = pathlib.Path(tmpdir)
            path_w1 = generate_partitioned_dataset(
                output_dir=out_dir,
                n=120,
                d=20,
                p=6,
                seed=99,
                show_progress=False,
                workers=1,
            )
            with path_w1.open("r", encoding="utf-8") as f:
                payload_w1 = json.load(f)

            path_w2 = generate_partitioned_dataset(
                output_dir=out_dir,
                n=120,
                d=20,
                p=6,
                seed=99,
                show_progress=False,
                workers=2,
            )
            with path_w2.open("r", encoding="utf-8") as f:
                payload_w2 = json.load(f)

            self.assertEqual(payload_w1, payload_w2)


if __name__ == "__main__":
    unittest.main()
