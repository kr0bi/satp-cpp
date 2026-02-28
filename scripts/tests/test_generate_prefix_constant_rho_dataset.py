#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import struct
import sys
import tempfile
import unittest
import zlib

SCRIPT_DIR = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

from generate_prefix_constant_rho_dataset_bin import (  # noqa: E402
    ENCODING_ZLIB_BITSET_LE,
    ENCODING_ZLIB_U32_LE,
    HEADER_FMT,
    MAGIC,
    PARTITION_ENTRY_FMT,
    VERSION,
    generate_prefix_constant_rho_dataset_bin,
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
            _d_part,
            values_encoding,
            truth_encoding,
            _reserved,
        ) in entries:
            f.seek(values_offset)
            payload = f.read(values_byte_size)
            if values_encoding != ENCODING_ZLIB_U32_LE:
                raise ValueError("Unsupported values encoding")
            raw_payload = zlib.decompress(payload)
            values = [v[0] for v in struct.iter_unpack("<I", raw_payload)]
            if len(values) != n_part:
                raise ValueError("Partition row count mismatch")
            partitions.append(values)

            f.seek(truth_offset)
            truth_payload = f.read(truth_byte_size)
            if truth_encoding != ENCODING_ZLIB_BITSET_LE:
                raise ValueError("Unsupported truth encoding")
            truth_raw = zlib.decompress(truth_payload)
            expected_truth_size = (n_part + 7) // 8
            if len(truth_raw) != expected_truth_size:
                raise ValueError("Corrupted truth payload size")
            truth_flags_per_partition.append(_unpack_truth_flags(truth_raw, n_part))

    return (n, d, p, seed), entries, partitions, truth_flags_per_partition


class GeneratePrefixConstantRhoDatasetTests(unittest.TestCase):
    def test_schema_and_counts(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = pathlib.Path(tmpdir)
            out_path = generate_prefix_constant_rho_dataset_bin(
                output_dir=out_dir,
                n=120,
                d=12,
                p=3,
                seed=21041998,
                show_progress=False,
            )

            self.assertEqual(out_path.name, "dataset_n_120_d_12_p_3_s_21041998.bin")
            header, entries, partitions, truth = read_binary_dataset(out_path)
            n, d, p, seed = header
            self.assertEqual((n, d, p, seed), (120, 12, 3, 21041998))
            self.assertEqual(len(entries), 3)

            for values, flags in zip(partitions, truth):
                self.assertEqual(len(values), n)
                self.assertEqual(len(flags), n)
                self.assertEqual(len(set(values)), d)
                self.assertEqual(sum(flags), d)

    def test_constant_rho_prefix_boundaries(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            n = 1000
            d = 100
            rho = n // d
            out_path = generate_prefix_constant_rho_dataset_bin(
                output_dir=pathlib.Path(tmpdir),
                n=n,
                d=d,
                p=2,
                seed=7,
                show_progress=False,
            )

            _, _, _, truth = read_binary_dataset(out_path)
            for flags in truth:
                # At every block boundary t = j * rho, we expect F0(t) = j.
                prefix = 0
                for t in range(1, n + 1):
                    prefix += flags[t - 1]
                    if t % rho == 0:
                        j = t // rho
                        self.assertEqual(prefix, j)

                # Exactly one first occurrence per rho-sized block.
                for block in range(d):
                    start = block * rho
                    end = start + rho
                    self.assertEqual(sum(flags[start:end]), 1)

    def test_reject_non_divisible_n_d(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            with self.assertRaises(ValueError):
                generate_prefix_constant_rho_dataset_bin(
                    output_dir=pathlib.Path(tmpdir),
                    n=101,
                    d=10,
                    p=1,
                    seed=1,
                    show_progress=False,
                )

    def test_deterministic_across_workers(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = pathlib.Path(tmpdir)
            p1 = generate_prefix_constant_rho_dataset_bin(
                output_dir=out_dir, n=500, d=50, p=4, seed=42, workers=1, show_progress=False)
            p2 = generate_prefix_constant_rho_dataset_bin(
                output_dir=out_dir, n=500, d=50, p=4, seed=42, workers=2, show_progress=False)

            with p1.open("rb") as f1, p2.open("rb") as f2:
                self.assertEqual(f1.read(), f2.read())


if __name__ == "__main__":
    unittest.main()
