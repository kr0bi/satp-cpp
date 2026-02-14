#!/usr/bin/env python3
from __future__ import annotations

import argparse
import math
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple

import numpy as np
import pandas as pd
import zlib

MAGIC = b"SATPDBN2"
VERSION = 2
HEADER_FMT = "<8sIQQQQ"
PARTITION_ENTRY_FMT = "<QQQQQQIII"
HEADER_SIZE = struct.calcsize(HEADER_FMT)
ENTRY_SIZE = struct.calcsize(PARTITION_ENTRY_FMT)


@dataclass(frozen=True)
class PartitionEntry:
    values_offset: int
    values_byte_size: int
    truth_offset: int
    truth_byte_size: int
    n: int
    d: int
    values_encoding: int
    truth_encoding: int
    reserved: int


@dataclass(frozen=True)
class DatasetIndex:
    path: Path
    n: int
    d: int
    p: int
    seed: int
    entries: Tuple[PartitionEntry, ...]
    file_size: int


def iter_dataset_files(datasets_root: Path) -> List[Path]:
    return sorted(datasets_root.rglob("dataset_n_*_d_*_p_*_s_*.bin"))


def read_dataset_index(path: Path) -> DatasetIndex:
    with path.open("rb") as f:
        header_raw = f.read(HEADER_SIZE)
        if len(header_raw) != HEADER_SIZE:
            raise ValueError(f"{path}: invalid header size")
        magic, version, n, d, p, seed = struct.unpack(HEADER_FMT, header_raw)
        if magic != MAGIC:
            raise ValueError(f"{path}: invalid magic")
        if version != VERSION:
            raise ValueError(f"{path}: unsupported version {version}")

        entries: List[PartitionEntry] = []
        for _ in range(p):
            raw = f.read(ENTRY_SIZE)
            if len(raw) != ENTRY_SIZE:
                raise ValueError(f"{path}: invalid partition table")
            e = PartitionEntry(*struct.unpack(PARTITION_ENTRY_FMT, raw))
            entries.append(e)

    file_size = path.stat().st_size
    return DatasetIndex(path=path, n=n, d=d, p=p, seed=seed, entries=tuple(entries), file_size=file_size)


def _safe_ratio(num: float, den: float) -> float:
    if den == 0:
        return 0.0
    return float(num) / float(den)


def summarize_index(idx: DatasetIndex) -> Dict[str, float]:
    values_sizes = np.array([e.values_byte_size for e in idx.entries], dtype=np.float64)
    truth_sizes = np.array([e.truth_byte_size for e in idx.entries], dtype=np.float64)
    raw_values = float(sum(e.n * 4 for e in idx.entries))
    raw_truth = float(sum(((e.n + 7) // 8) for e in idx.entries))
    raw_total = raw_values + raw_truth
    compressed_values = float(values_sizes.sum())
    compressed_truth = float(truth_sizes.sum())
    compressed_total = compressed_values + compressed_truth

    return {
        "path": str(idx.path),
        "file_name": idx.path.name,
        "n": idx.n,
        "d": idx.d,
        "p": idx.p,
        "seed": idx.seed,
        "d_over_n": _safe_ratio(idx.d, idx.n),
        "file_size_bytes": idx.file_size,
        "raw_values_bytes": raw_values,
        "raw_truth_bytes": raw_truth,
        "raw_total_bytes": raw_total,
        "compressed_values_bytes": compressed_values,
        "compressed_truth_bytes": compressed_truth,
        "compressed_total_bytes": compressed_total,
        "overall_compression_ratio": _safe_ratio(compressed_total, raw_total),
        "values_compression_ratio": _safe_ratio(compressed_values, raw_values),
        "truth_compression_ratio": _safe_ratio(compressed_truth, raw_truth),
        "values_part_size_mean": float(values_sizes.mean()) if len(values_sizes) else 0.0,
        "values_part_size_std": float(values_sizes.std(ddof=0)) if len(values_sizes) else 0.0,
        "values_part_size_min": float(values_sizes.min()) if len(values_sizes) else 0.0,
        "values_part_size_max": float(values_sizes.max()) if len(values_sizes) else 0.0,
        "truth_part_size_mean": float(truth_sizes.mean()) if len(truth_sizes) else 0.0,
        "truth_part_size_std": float(truth_sizes.std(ddof=0)) if len(truth_sizes) else 0.0,
        "truth_part_size_min": float(truth_sizes.min()) if len(truth_sizes) else 0.0,
        "truth_part_size_max": float(truth_sizes.max()) if len(truth_sizes) else 0.0,
    }


def build_inventory(datasets_root: Path) -> pd.DataFrame:
    rows: List[Dict[str, float]] = []
    for path in iter_dataset_files(datasets_root):
        idx = read_dataset_index(path)
        rows.append(summarize_index(idx))
    if not rows:
        return pd.DataFrame()
    return pd.DataFrame(rows).sort_values(["n", "d", "seed", "p"]).reset_index(drop=True)


def _load_partition_values(idx: DatasetIndex, part_idx: int) -> np.ndarray:
    e = idx.entries[part_idx]
    with idx.path.open("rb") as f:
        f.seek(e.values_offset)
        compressed = f.read(e.values_byte_size)
    raw = zlib.decompress(compressed)
    expected = e.n * 4
    if len(raw) != expected:
        raise ValueError(f"{idx.path}: partition {part_idx} values size mismatch ({len(raw)} vs {expected})")
    return np.frombuffer(raw, dtype="<u4")


def _load_partition_truth_bits(idx: DatasetIndex, part_idx: int) -> np.ndarray:
    e = idx.entries[part_idx]
    with idx.path.open("rb") as f:
        f.seek(e.truth_offset)
        compressed = f.read(e.truth_byte_size)
    raw = zlib.decompress(compressed)
    expected = (e.n + 7) // 8
    if len(raw) != expected:
        raise ValueError(f"{idx.path}: partition {part_idx} truth size mismatch ({len(raw)} vs {expected})")
    bits = np.unpackbits(np.frombuffer(raw, dtype=np.uint8), bitorder="little")
    return bits[: e.n].astype(np.uint8, copy=False)


def _entropy_from_counts(counts: np.ndarray) -> float:
    total = float(counts.sum())
    if total <= 0:
        return 0.0
    p = counts[counts > 0].astype(np.float64) / total
    return float(-(p * np.log2(p)).sum())


def _gini_impurity_from_counts(counts: np.ndarray) -> float:
    total = float(counts.sum())
    if total <= 0:
        return 0.0
    p = counts[counts > 0].astype(np.float64) / total
    return float(1.0 - np.square(p).sum())


def _lag1_corr(values: np.ndarray) -> float:
    if len(values) < 3:
        return 0.0
    x = values[:-1].astype(np.float64, copy=False)
    y = values[1:].astype(np.float64, copy=False)
    x_std = x.std()
    y_std = y.std()
    if x_std == 0.0 or y_std == 0.0:
        return 0.0
    return float(np.corrcoef(x, y)[0, 1])


def _partition_frequency_stats(values: np.ndarray, domain_size: int) -> Dict[str, float]:
    counts = np.bincount(values, minlength=domain_size)
    nz = counts[counts > 0]
    mean_nz = float(nz.mean()) if len(nz) else 0.0
    std_nz = float(nz.std(ddof=0)) if len(nz) else 0.0
    return {
        "unique_observed": int(len(nz)),
        "freq_min": int(nz.min()) if len(nz) else 0,
        "freq_max": int(nz.max()) if len(nz) else 0,
        "freq_mean": mean_nz,
        "freq_std": std_nz,
        "freq_cv": _safe_ratio(std_nz, mean_nz),
        "entropy_bits": _entropy_from_counts(counts),
        "gini_impurity": _gini_impurity_from_counts(counts),
        "top1_freq": int(np.partition(nz, -1)[-1]) if len(nz) else 0,
        "top5_freq_sum": int(np.sort(nz)[-5:].sum()) if len(nz) >= 5 else int(nz.sum()) if len(nz) else 0,
        "equal_adjacent_ratio": float(np.mean(values[:-1] == values[1:])) if len(values) > 1 else 0.0,
        "lag1_corr": _lag1_corr(values),
    }


def _f0_curve_from_truth(truth_bits: np.ndarray, d: int, checkpoints: int = 200) -> pd.DataFrame:
    n = len(truth_bits)
    if n == 0:
        return pd.DataFrame(columns=["t", "f0_true", "f0_expected_uniform", "f0_delta"])
    csum = np.cumsum(truth_bits.astype(np.int64))
    k = min(checkpoints, n)
    t = np.unique(np.ceil(np.linspace(1, n, k)).astype(np.int64))
    f0_true = csum[t - 1]
    if d > 0:
        f0_exp = d * (1.0 - np.power(1.0 - (1.0 / d), t))
    else:
        f0_exp = np.zeros_like(t, dtype=np.float64)
    return pd.DataFrame({
        "t": t,
        "f0_true": f0_true.astype(np.float64),
        "f0_expected_uniform": f0_exp.astype(np.float64),
        "f0_delta": f0_true.astype(np.float64) - f0_exp.astype(np.float64),
    })


def _pairwise_jaccard(unique_arrays: List[np.ndarray]) -> pd.DataFrame:
    rows: List[Dict[str, float]] = []
    for i in range(len(unique_arrays)):
        for j in range(len(unique_arrays)):
            a = unique_arrays[i]
            b = unique_arrays[j]
            inter = np.intersect1d(a, b, assume_unique=True).size
            union = a.size + b.size - inter
            rows.append({
                "partition_i": i,
                "partition_j": j,
                "intersection": int(inter),
                "union": int(union),
                "jaccard": _safe_ratio(inter, union),
            })
    return pd.DataFrame(rows)


def partition_frequency_histogram(path: Path, partition: int) -> pd.DataFrame:
    idx = read_dataset_index(path)
    if partition < 0 or partition >= idx.p:
        raise ValueError(f"partition out of range: {partition}")
    values = _load_partition_values(idx, partition)
    counts = np.bincount(values, minlength=idx.d)
    nz = counts[counts > 0]
    if len(nz) == 0:
        return pd.DataFrame(columns=["partition", "frequency", "id_count"])
    freq, id_count = np.unique(nz, return_counts=True)
    return pd.DataFrame({
        "partition": partition,
        "frequency": freq.astype(np.int64),
        "id_count": id_count.astype(np.int64),
    })


def partition_first_occurrence_positions(path: Path, partition: int) -> pd.DataFrame:
    idx = read_dataset_index(path)
    if partition < 0 or partition >= idx.p:
        raise ValueError(f"partition out of range: {partition}")
    truth = _load_partition_truth_bits(idx, partition)
    pos = np.flatnonzero(truth) + 1
    return pd.DataFrame({
        "partition": partition,
        "position": pos.astype(np.int64),
    })


def analyze_dataset(path: Path,
                    max_partitions: int = 3,
                    checkpoints: int = 200,
                    compute_overlap: bool = True) -> Dict[str, pd.DataFrame]:
    idx = read_dataset_index(path)
    max_partitions = max(1, min(max_partitions, idx.p))

    partition_rows: List[Dict[str, float]] = []
    freq_rows: List[Dict[str, float]] = []
    f0_rows: List[pd.DataFrame] = []
    unique_arrays: List[np.ndarray] = []

    for part_idx in range(max_partitions):
        e = idx.entries[part_idx]
        raw_values = e.n * 4
        raw_truth = (e.n + 7) // 8
        partition_rows.append({
            "partition": part_idx,
            "n": e.n,
            "d_declared": e.d,
            "values_bytes": e.values_byte_size,
            "truth_bytes": e.truth_byte_size,
            "raw_values_bytes": raw_values,
            "raw_truth_bytes": raw_truth,
            "values_ratio": _safe_ratio(e.values_byte_size, raw_values),
            "truth_ratio": _safe_ratio(e.truth_byte_size, raw_truth),
        })

        values = _load_partition_values(idx, part_idx)
        truth_bits = _load_partition_truth_bits(idx, part_idx)

        freq = _partition_frequency_stats(values, idx.d)
        freq["partition"] = part_idx
        freq["truth_ones"] = int(truth_bits.sum())
        freq["truth_ones_ratio"] = _safe_ratio(int(truth_bits.sum()), len(truth_bits))
        freq["d_declared"] = idx.d
        freq["truth_consistent"] = int(truth_bits.sum()) == idx.d
        freq_rows.append(freq)

        f0_df = _f0_curve_from_truth(truth_bits, idx.d, checkpoints=checkpoints)
        f0_df["partition"] = part_idx
        f0_rows.append(f0_df)

        if compute_overlap:
            unique_arrays.append(np.unique(values))

    out: Dict[str, pd.DataFrame] = {
        "inventory_row": pd.DataFrame([summarize_index(idx)]),
        "partition_sizes": pd.DataFrame(partition_rows),
        "partition_stats": pd.DataFrame(freq_rows).sort_values("partition"),
        "f0_curves": pd.concat(f0_rows, ignore_index=True) if f0_rows else pd.DataFrame(),
    }
    if compute_overlap and unique_arrays:
        out["overlap_jaccard"] = _pairwise_jaccard(unique_arrays)
    else:
        out["overlap_jaccard"] = pd.DataFrame()
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description="Statistical exploration for SATP binary datasets.")
    parser.add_argument("--datasets-root", type=Path, default=Path("datasets"), help="Root folder containing .bin datasets")
    parser.add_argument("--inventory-csv", type=Path, default=None, help="Optional output CSV for dataset inventory")
    parser.add_argument("--dataset", type=Path, default=None, help="Single dataset file for deep analysis")
    parser.add_argument("--out-dir", type=Path, default=Path("results/dataset_exploration"), help="Output folder for deep analysis CSVs")
    parser.add_argument("--max-partitions", type=int, default=3, help="Max partitions to inspect deeply")
    parser.add_argument("--checkpoints", type=int, default=200, help="Number of F0(t) checkpoints")
    args = parser.parse_args()

    inventory = build_inventory(args.datasets_root)
    if inventory.empty:
        raise SystemExit(f"No datasets found under {args.datasets_root}")

    if args.inventory_csv is not None:
        args.inventory_csv.parent.mkdir(parents=True, exist_ok=True)
        inventory.to_csv(args.inventory_csv, index=False)
        print(args.inventory_csv)
    else:
        print(inventory.head(20).to_string(index=False))

    if args.dataset is not None:
        out = analyze_dataset(
            args.dataset,
            max_partitions=args.max_partitions,
            checkpoints=args.checkpoints,
            compute_overlap=True,
        )
        args.out_dir.mkdir(parents=True, exist_ok=True)
        base = args.dataset.stem
        for key, df in out.items():
            path = args.out_dir / f"{base}_{key}.csv"
            df.to_csv(path, index=False)
            print(path)


if __name__ == "__main__":
    main()
