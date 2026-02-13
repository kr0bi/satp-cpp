#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
from typing import Iterable

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


def _numeric_columns(df: pd.DataFrame, cols: Iterable[str]) -> None:
    for col in cols:
        df[col] = pd.to_numeric(df[col], errors="coerce")


def _plot_metric(
    df: pd.DataFrame,
    metric: str,
    ylabel: str,
    out_path: pathlib.Path,
    *,
    logx: bool = True,
    logy: bool = False,
    title: str | None = None,
) -> None:
    fig, ax = plt.subplots(figsize=(10, 5.5))
    for algo, g in df.groupby("algorithm", sort=True):
        g = g.sort_values("sample_size")
        ax.plot(g["sample_size"], g[metric], marker="o", linewidth=2, label=algo)

    if logx:
        ax.set_xscale("log")
    if logy:
        ax.set_yscale("log")

    ax.set_xlabel("sample_size")
    ax.set_ylabel(ylabel)
    ax.set_title(title or f"{metric} vs sample_size")
    ax.grid(True, which="both", alpha=0.25)
    ax.legend()
    fig.tight_layout()
    fig.savefig(out_path, dpi=160)
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate benchmark plots from SATP CSV results.")
    parser.add_argument("--csv", type=pathlib.Path, default=pathlib.Path("results.csv"),
                        help="Input CSV path (default: results.csv)")
    parser.add_argument("--out-dir", type=pathlib.Path, default=pathlib.Path("plots"),
                        help="Output directory for generated plots (default: plots)")
    args = parser.parse_args()

    if not args.csv.exists():
        raise SystemExit(f"CSV not found: {args.csv}")

    args.out_dir.mkdir(parents=True, exist_ok=True)

    df = pd.read_csv(args.csv)
    _numeric_columns(df, [
        "sample_size",
        "f0_mean",
        "f0_hat_mean",
        "variance",
        "stddev",
        "rse_theoretical",
        "rse_observed",
        "bias",
        "difference",
        "bias_relative",
        "mean_relative_error",
        "rmse",
        "mae",
    ])

    # 1) Cardinality estimate vs sample size + ground-truth line
    fig, ax = plt.subplots(figsize=(10, 5.5))
    for algo, g in df.groupby("algorithm", sort=True):
        g = g.sort_values("sample_size")
        ax.plot(g["sample_size"], g["f0_hat_mean"], marker="o", linewidth=2, label=f"{algo} (F0_hat)")
    gt = df.groupby("sample_size", as_index=False)["f0_mean"].mean().sort_values("sample_size")
    ax.plot(gt["sample_size"], gt["f0_mean"], linestyle="--", linewidth=2.0, color="black", label="F0 vero")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("sample_size")
    ax.set_ylabel("f0_hat_mean")
    ax.set_title("Stima media F0 vs sample_size")
    ax.grid(True, which="both", alpha=0.25)
    ax.legend()
    fig.tight_layout()
    fig.savefig(args.out_dir / "f0_hat_mean_vs_sample_size.png", dpi=160)
    plt.close(fig)

    # 2) Error metrics
    _plot_metric(df, "mean_relative_error", "MRE", args.out_dir / "mre_vs_sample_size.png",
                 logx=True, logy=True, title="Errore relativo medio (MRE)")
    _plot_metric(df, "bias_relative", "Bias relativo", args.out_dir / "bias_relative_vs_sample_size.png",
                 logx=True, logy=False, title="Bias relativo (signed)")
    _plot_metric(df, "difference", "|Bias|", args.out_dir / "abs_bias_vs_sample_size.png",
                 logx=True, logy=True, title="Bias assoluto")
    _plot_metric(df, "rmse", "RMSE", args.out_dir / "rmse_vs_sample_size.png",
                 logx=True, logy=True, title="RMSE vs sample_size")
    _plot_metric(df, "mae", "MAE", args.out_dir / "mae_vs_sample_size.png",
                 logx=True, logy=True, title="MAE vs sample_size")
    _plot_metric(df, "stddev", "Stddev", args.out_dir / "stddev_vs_sample_size.png",
                 logx=True, logy=True, title="Deviazione standard vs sample_size")
    _plot_metric(df, "variance", "Varianza", args.out_dir / "variance_vs_sample_size.png",
                 logx=True, logy=True, title="Varianza vs sample_size")
    _plot_metric(df, "rse_observed", "RSE osservata", args.out_dir / "rse_observed_vs_sample_size.png",
                 logx=True, logy=True, title="RSE osservata vs sample_size")

    # 3) RSE observed vs theoretical
    fig, ax = plt.subplots(figsize=(10, 5.5))
    for algo, g in df.groupby("algorithm", sort=True):
        g = g.sort_values("sample_size")
        ax.plot(g["sample_size"], g["rse_observed"], marker="o", linewidth=2, label=f"{algo} (obs)")
        gt = g.dropna(subset=["rse_theoretical"])
        if not gt.empty:
            ax.plot(gt["sample_size"], gt["rse_theoretical"], linestyle="--", linewidth=1.8, label=f"{algo} (theory)")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("sample_size")
    ax.set_ylabel("RSE")
    ax.set_title("RSE osservata vs teorica")
    ax.grid(True, which="both", alpha=0.25)
    ax.legend(ncol=2)
    fig.tight_layout()
    fig.savefig(args.out_dir / "rse_observed_vs_theoretical.png", dpi=160)
    plt.close(fig)

    # 4) RSE ratio obs/theory where available
    ratio_df = df.dropna(subset=["rse_theoretical"]).copy()
    ratio_df = ratio_df[ratio_df["rse_theoretical"] > 0.0]
    ratio_df["rse_ratio"] = ratio_df["rse_observed"] / ratio_df["rse_theoretical"]
    _plot_metric(
        ratio_df,
        "rse_ratio",
        "RSE_obs / RSE_theory",
        args.out_dir / "rse_ratio_obs_over_theory.png",
        logx=True,
        logy=True,
        title="Rapporto RSE osservata / teorica",
    )

    # 5) Estimate vs truth scatter
    fig, ax = plt.subplots(figsize=(7.5, 7.5))
    for algo, g in df.groupby("algorithm", sort=True):
        ax.scatter(g["f0_mean"], g["f0_hat_mean"], s=45, alpha=0.9, label=algo)
    min_v = min(float(df["f0_mean"].min()), float(df["f0_hat_mean"].min()))
    max_v = max(float(df["f0_mean"].max()), float(df["f0_hat_mean"].max()))
    ref = np.geomspace(max(min_v, 1e-9), max_v, 200)
    ax.plot(ref, ref, color="black", linestyle="--", linewidth=1.5, label="y = x")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("F0 vero (f0_mean)")
    ax.set_ylabel("F0 stimato (f0_hat_mean)")
    ax.set_title("F0 stimato vs F0 vero")
    ax.grid(True, which="both", alpha=0.25)
    ax.legend()
    fig.tight_layout()
    fig.savefig(args.out_dir / "f0_hat_vs_f0_true_scatter.png", dpi=170)
    plt.close(fig)

    print(f"Generated plots in: {args.out_dir}")


if __name__ == "__main__":
    main()
