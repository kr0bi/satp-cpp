#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest
import struct
import zlib

SCRIPT_DIR = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

from generate_partitioned_dataset_bin import (  # noqa: E402
    generate_partitioned_dataset_bin,
    MAGIC,
    VERSION,
    ENCODING_ZLIB_U32_LE,
    ENCODING_ZLIB_BITSET_LE,
    HEADER_FMT,
    PARTITION_ENTRY_FMT,
)


def _unpack_truth_flags(truth_raw: bytes, n_part: int) -> list[int]:
    out: list[int] = []
    for i in range(n_part):
        byte = truth_raw[i >> 3]
        out.append((byte >> (i & 7)) & 0x1)
    return out


def read_binary_dataset(path: pathlib.Path) -> tuple[
    tuple[int, int, int, int],
    list[tuple[int, int, int, int, int, int, int, int, int]],
    list[list[int]],
    list[list[int]],
]:
    header_size = struct.calcsize(HEADER_FMT)
    entry_size = struct.calcsize(PARTITION_ENTRY_FMT)

    with path.open("rb") as f:
        header_raw = f.read(header_size)
        if len(header_raw) != header_size:
            raise ValueError("Invalid binary header")

        magic, version, n, d, p, seed = struct.unpack(HEADER_FMT, header_raw)
        if magic != MAGIC:
            raise ValueError("Invalid binary magic")
        if version != VERSION:
            raise ValueError("Unsupported binary version")

        entries: list[tuple[int, int, int, int, int, int, int, int, int]] = []
        for _ in range(p):
            raw_entry = f.read(entry_size)
            if len(raw_entry) != entry_size:
                raise ValueError("Invalid partition table")
            entries.append(struct.unpack(PARTITION_ENTRY_FMT, raw_entry))

        partitions: list[list[int]] = []
        truth_flags_per_partition: list[list[int]] = []
        for (
            values_offset,
            values_byte_size,
            truth_offset,
            truth_byte_size,
            n_part,
            d_part,
            values_encoding,
            truth_encoding,
            _reserved
        ) in entries:
            f.seek(values_offset)
            payload = f.read(values_byte_size)
            if len(payload) != values_byte_size:
                raise ValueError("Corrupted partition payload")

            if values_encoding == ENCODING_ZLIB_U32_LE:
                raw_payload = zlib.decompress(payload)
            else:
                raise ValueError("Unsupported encoding")

            if len(raw_payload) != n_part * 4:
                raise ValueError("Corrupted decompressed partition payload size")
            values = [v[0] for v in struct.iter_unpack("<I", raw_payload)]
            if len(values) != n_part:
                raise ValueError("Partition row count mismatch")
            partitions.append(values)

            f.seek(truth_offset)
            truth_payload = f.read(truth_byte_size)
            if len(truth_payload) != truth_byte_size:
                raise ValueError("Corrupted truth payload")
            if truth_encoding != ENCODING_ZLIB_BITSET_LE:
                raise ValueError("Unsupported truth encoding")
            truth_raw = zlib.decompress(truth_payload)
            expected_truth_size = (n_part + 7) // 8
            if len(truth_raw) != expected_truth_size:
                raise ValueError("Corrupted decompressed truth payload size")
            truth_flags_per_partition.append(_unpack_truth_flags(truth_raw, n_part))

    return (n, d, p, seed), entries, partitions, truth_flags_per_partition


class GenerateDatasetTests(unittest.TestCase):

    def test_partitioned_binary_schema_and_counts(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = pathlib.Path(tmpdir)
            out_path = generate_partitioned_dataset_bin(
                output_dir=out_dir,
                n=30,
                d=7,
                p=4,
                seed=123,
                show_progress=False,
            )

            self.assertEqual(out_path.name, "dataset_n_30_d_7_p_4_s_123.bin")
            self.assertTrue(out_path.exists())

            header, entries, partitions, truth_flags_per_partition = read_binary_dataset(out_path)
            n, d, p, seed = header
            self.assertEqual((n, d, p, seed), (30, 7, 4, 123))
            self.assertEqual(len(entries), 4)
            self.assertEqual(len(partitions), 4)
            self.assertEqual(len(truth_flags_per_partition), 4)

            total_items = 0
            entry_size = struct.calcsize(PARTITION_ENTRY_FMT)
            first_data_offset = struct.calcsize(HEADER_FMT) + p * entry_size
            expected_offset = first_data_offset

            for (
                values_offset,
                values_byte_size,
                truth_offset,
                truth_byte_size,
                n_part,
                d_part,
                values_encoding,
                truth_encoding,
                reserved
            ), values, truth_flags in zip(entries, partitions, truth_flags_per_partition):
                self.assertEqual(values_offset, expected_offset)
                self.assertGreater(values_byte_size, 0)
                self.assertLessEqual(values_byte_size, n * 4)
                expected_offset += values_byte_size

                self.assertEqual(truth_offset, expected_offset)
                self.assertGreater(truth_byte_size, 0)
                expected_offset += truth_byte_size

                self.assertEqual(n_part, n)
                self.assertEqual(d_part, d)
                self.assertEqual(values_encoding, ENCODING_ZLIB_U32_LE)
                self.assertEqual(truth_encoding, ENCODING_ZLIB_BITSET_LE)
                self.assertEqual(reserved, 0)
                self.assertEqual(len(values), n)
                self.assertEqual(len(set(values)), d)

                # truth flags represent first occurrence positions:
                # prefix sum equals the exact distinct count at each t.
                self.assertEqual(len(truth_flags), n)
                prefix = 0
                seen: set[int] = set()
                for idx, value in enumerate(values):
                    if value not in seen:
                        seen.add(value)
                        self.assertEqual(truth_flags[idx], 1)
                    else:
                        self.assertEqual(truth_flags[idx], 0)
                    prefix += truth_flags[idx]
                self.assertEqual(prefix, d)

                total_items += len(values)

            self.assertEqual(total_items, n * p)

    def test_partitioned_binary_invalid_params(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = pathlib.Path(tmpdir)
            with self.assertRaises(ValueError):
                generate_partitioned_dataset_bin(
                    output_dir=out_dir,
                    n=10,
                    d=11,
                    p=2,
                    seed=1,
                    show_progress=False,
                )

    def test_partitioned_binary_deterministic_across_workers(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = pathlib.Path(tmpdir)
            path_w1 = generate_partitioned_dataset_bin(
                output_dir=out_dir,
                n=120,
                d=20,
                p=6,
                seed=99,
                show_progress=False,
                workers=1,
            )
            path_w2 = generate_partitioned_dataset_bin(
                output_dir=out_dir,
                n=120,
                d=20,
                p=6,
                seed=99,
                show_progress=False,
                workers=2,
            )

            with path_w1.open("rb") as f1, path_w2.open("rb") as f2:
                self.assertEqual(f1.read(), f2.read())


if __name__ == "__main__":
    unittest.main()
