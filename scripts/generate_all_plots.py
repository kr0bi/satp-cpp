#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
from typing import Iterable

import numpy as np
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go


NUMERIC_COLS = [
    "runs",
    "sample_size",
    "number_of_elements_processed",
    "f0",
    "seed",
    "f0_mean_t",
    "f0_heat_mean_t",
    "variance",
    "stddev",
    "rse_theoretical",
    "rse_observed",
    "bias",
    "absolute_bias",
    "relative_bias",
    "mean_relative_error",
    "rmse",
    "mae",
]


def load_results(results_path: Path) -> pd.DataFrame:
    df = pd.read_csv(results_path)
    if "element_index" in df.columns and "number_of_elements_processed" not in df.columns:
        df = df.rename(columns={"element_index": "number_of_elements_processed"})
    missing = {"algorithm", "mode", "sample_size", "number_of_elements_processed"} - set(df.columns)
    if missing:
        raise ValueError(f"results.csv missing required columns: {sorted(missing)}")

    for col in NUMERIC_COLS:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")

    df["algorithm"] = df["algorithm"].astype("category")
    df["mode"] = df["mode"].astype("category")
    return df


def _write(fig: go.Figure, out_path: Path) -> None:
    fig.update_layout(template="plotly_white", height=520, width=980)
    fig.write_html(out_path, include_plotlyjs="cdn")


def generate_normal_plots(normal: pd.DataFrame, out_dir: Path) -> list[Path]:
    generated: list[Path] = []
    metrics = [
        "f0_heat_mean_t",
        "bias",
        "absolute_bias",
        "variance",
        "stddev",
        "rmse",
        "mae",
        "mean_relative_error",
        "relative_bias",
        "rse_observed",
        "rse_theoretical",
    ]

    for metric in metrics:
        fig = px.line(
            normal.sort_values(["algorithm", "sample_size"]),
            x="sample_size",
            y=metric,
            color="algorithm",
            markers=True,
            log_x=True,
            title=f"Normal mode: {metric} vs sample_size",
            hover_data=["f0_mean_t", "f0_heat_mean_t", "runs", "seed", "params"],
        )
        out_path = out_dir / f"normal_{metric}.html"
        _write(fig, out_path)
        generated.append(out_path)

    # RSE observed vs theoretical
    rse_df = normal.dropna(subset=["rse_observed", "rse_theoretical"])
    rse_df = rse_df[np.isfinite(rse_df["rse_theoretical"])]
    fig = go.Figure()
    for algo, g in rse_df.groupby("algorithm", observed=True):
        fig.add_trace(
            go.Scatter(
                x=g["sample_size"],
                y=g["rse_observed"],
                mode="lines+markers",
                name=f"{algo} (observed)",
                customdata=np.stack([g["rse_theoretical"]], axis=-1),
                hovertemplate=(
                    "algo=%{fullData.name}<br>"
                    "sample_size=%{x}<br>"
                    "rse_observed=%{y:.6f}<br>"
                    "rse_theoretical=%{customdata[0]:.6f}<extra></extra>"
                ),
            )
        )
        fig.add_trace(
            go.Scatter(
                x=g["sample_size"],
                y=g["rse_theoretical"],
                mode="lines+markers",
                name=f"{algo} (theory)",
                line=dict(dash="dot"),
                hovertemplate="algo=%{fullData.name}<br>sample_size=%{x}<br>rse_theoretical=%{y:.6f}<extra></extra>",
            )
        )
    fig.update_xaxes(type="log")
    fig.update_layout(title="Normal mode: RSE observed vs theoretical")
    out_path = out_dir / "normal_rse_observed_vs_theoretical.html"
    _write(fig, out_path)
    generated.append(out_path)

    # Calibration plot f0_heat_mean_t vs f0_mean_t
    fig = px.scatter(
        normal,
        x="f0_mean_t",
        y="f0_heat_mean_t",
        color="algorithm",
        symbol="algorithm",
        log_x=True,
        log_y=True,
        title="Normal mode: calibration f0_heat_mean_t vs f0_mean_t",
        hover_data=["sample_size", "absolute_bias", "rmse", "mae", "mean_relative_error", "params"],
    )
    min_xy = min(normal["f0_mean_t"].min(), normal["f0_heat_mean_t"].min())
    max_xy = max(normal["f0_mean_t"].max(), normal["f0_heat_mean_t"].max())
    fig.add_trace(
        go.Scatter(
            x=[min_xy, max_xy],
            y=[min_xy, max_xy],
            mode="lines",
            name="y=x",
            line=dict(dash="dash", color="black"),
        )
    )
    out_path = out_dir / "normal_calibration_f0_heat_mean_t_vs_f0_mean_t.html"
    _write(fig, out_path)
    generated.append(out_path)

    return generated


def downsample_streaming(stream: pd.DataFrame, n_bins: int = 400) -> pd.DataFrame:
    out = stream.copy()
    out["progress"] = out["number_of_elements_processed"] / out["sample_size"]
    out["progress_bin"] = (out["progress"] * n_bins).round() / n_bins
    metrics = [
        "f0_mean_t",
        "f0_heat_mean_t",
        "variance",
        "stddev",
        "rse_observed",
        "bias",
        "absolute_bias",
        "relative_bias",
        "mean_relative_error",
        "rmse",
        "mae",
    ]
    reduced = (
        out.groupby(["algorithm", "sample_size", "progress_bin"], as_index=False, observed=True)[metrics]
        .mean()
    )
    reduced["number_of_elements_processed"] = (
        (reduced["progress_bin"] * reduced["sample_size"])
        .round()
        .clip(lower=1)
        .astype(int)
    )
    return reduced


def generate_streaming_plots(stream: pd.DataFrame, out_dir: Path) -> list[Path]:
    generated: list[Path] = []
    reduced = downsample_streaming(stream, n_bins=400)

    metrics = [
        "f0_heat_mean_t",
        "bias",
        "absolute_bias",
        "variance",
        "stddev",
        "rmse",
        "mae",
        "mean_relative_error",
        "relative_bias",
        "rse_observed",
    ]

    for metric in metrics:
        fig = px.line(
            reduced.sort_values(["sample_size", "progress_bin", "algorithm"]),
            x="progress_bin",
            y=metric,
            color="algorithm",
            facet_col="sample_size",
            facet_col_wrap=3,
            title=f"Streaming mode: {metric} vs progress (downsampled)",
            hover_data=["number_of_elements_processed", "f0_mean_t", "f0_heat_mean_t"],
        )
        fig.for_each_xaxis(lambda ax: ax.update(title="progress (t / sample_size)"))
        out_path = out_dir / f"streaming_{metric}_vs_progress.html"
        _write(fig, out_path)
        generated.append(out_path)

    max_n = int(reduced["sample_size"].max())
    reduced_max = reduced[reduced["sample_size"] == max_n].copy()
    for metric in metrics:
        fig = px.line(
            reduced_max.sort_values(["algorithm", "number_of_elements_processed"]),
            x="number_of_elements_processed",
            y=metric,
            color="algorithm",
            log_x=True,
            title=f"Streaming mode (sample_size={max_n}): {metric} vs number_of_elements_processed",
            hover_data=["progress_bin", "f0_mean_t", "f0_heat_mean_t"],
        )
        out_path = out_dir / f"streaming_{metric}_vs_number_of_elements_processed_maxN.html"
        _write(fig, out_path)
        generated.append(out_path)

    return generated


def generate_consistency_plots(normal: pd.DataFrame, stream: pd.DataFrame, out_dir: Path) -> list[Path]:
    generated: list[Path] = []
    stream_last = stream[stream["number_of_elements_processed"] == stream["sample_size"]].copy()
    join_cols = ["algorithm", "sample_size", "f0", "seed"]
    metrics = ["f0_heat_mean_t", "bias", "absolute_bias", "rmse", "mae", "mean_relative_error", "rse_observed"]
    merged = normal[join_cols + metrics].merge(
        stream_last[join_cols + metrics],
        on=join_cols,
        suffixes=("_normal", "_stream_last"),
        how="inner",
    )

    for metric in metrics:
        x_col = f"{metric}_normal"
        y_col = f"{metric}_stream_last"
        fig = px.scatter(
            merged,
            x=x_col,
            y=y_col,
            color="algorithm",
            symbol="algorithm",
            title=f"Consistency: {metric} normal vs streaming-last",
            hover_data=["sample_size"],
        )
        min_xy = min(merged[x_col].min(), merged[y_col].min())
        max_xy = max(merged[x_col].max(), merged[y_col].max())
        fig.add_trace(
            go.Scatter(
                x=[min_xy, max_xy],
                y=[min_xy, max_xy],
                mode="lines",
                name="y=x",
                line=dict(dash="dash", color="black"),
            )
        )
        out_path = out_dir / f"consistency_{metric}_normal_vs_stream_last.html"
        _write(fig, out_path)
        generated.append(out_path)

    return generated


def generate_all(results_path: Path, out_dir: Path) -> list[Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    df = load_results(results_path)
    normal = df[df["mode"] == "normal"].copy()
    stream = df[df["mode"] == "streaming"].copy()

    generated: list[Path] = []
    generated.extend(generate_normal_plots(normal, out_dir))
    generated.extend(generate_streaming_plots(stream, out_dir))
    generated.extend(generate_consistency_plots(normal, stream, out_dir))

    summary_path = out_dir / "plot_summary.txt"
    with summary_path.open("w", encoding="utf-8") as f:
        f.write(f"results_path={results_path}\n")
        f.write(f"rows_total={len(df)}\n")
        f.write(f"rows_normal={len(normal)}\n")
        f.write(f"rows_streaming={len(stream)}\n")
        f.write("sample_sizes=" + ",".join(map(str, sorted(normal["sample_size"].unique()))) + "\n")
        f.write("algorithms=" + ",".join(map(str, sorted(normal["algorithm"].astype(str).unique()))) + "\n")
        f.write(f"plots_generated={len(generated)}\n")
    generated.append(summary_path)

    return generated


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    results_path = root / "results.csv"
    out_dir = root / "plots"
    generated = generate_all(results_path, out_dir)
    for p in generated:
        print(p)


if __name__ == "__main__":
    main()
