#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


TARGET_N = 10_000_000
TARGET_SEED = 21041998
TARGET_RHO = [1, 10, 100]
TARGET_F0_STAR = [1_000, 2_000, 5_000, 10_000, 20_000, 50_000, 100_000]

ALGORITHM_SPECS = [
    {
        "key": "hllpp",
        "label": "HyperLogLog++",
        "param_name": "k",
        "results_dir": Path("results/streaming_precision_sweep_hllpp/streaming/hllpp/splitmix64"),
    },
    {
        "key": "hll",
        "label": "HyperLogLog",
        "param_name": "k",
        "results_dir": Path("results/streaming_precision_sweep_hll/streaming/hll/splitmix64"),
    },
    {
        "key": "ll",
        "label": "LogLog",
        "param_name": "k",
        "results_dir": Path("results/streaming_precision_sweep_ll/streaming/ll/splitmix64"),
    },
    {
        "key": "pc",
        "label": "Probabilistic Counting",
        "param_name": "L",
        "results_dir": Path("results/streaming_precision_sweep_pc/streaming/pc/splitmix64"),
    },
]


def find_repo_root(start: Path) -> Path:
    for candidate in (start, *start.parents):
        if (candidate / ".git").exists():
            return candidate
    raise RuntimeError("Repository root non trovata")


def extract_param_value(spec: dict[str, object], csv_path: Path) -> int:
    pattern = r"L_(\d+)" if spec["param_name"] == "L" else r"k_(\d+)"
    match = re.search(pattern, str(csv_path))
    if match is None:
        raise ValueError(f"Parametro non riconosciuto dal path: {csv_path}")
    return int(match.group(1))


def load_streaming_precision_data(repo_root: Path) -> pd.DataFrame:
    frames: list[pd.DataFrame] = []
    for spec in ALGORITHM_SPECS:
        base_dir = repo_root / spec["results_dir"]
        paths = sorted(base_dir.glob("*/results_streaming.csv"))
        if not paths:
            raise FileNotFoundError(f"Nessun CSV trovato in: {base_dir}")

        for csv_path in paths:
            frame = pd.read_csv(csv_path)
            frame["algorithm_label"] = spec["label"]
            frame["param_name"] = spec["param_name"]
            frame["param_value"] = extract_param_value(spec, csv_path)
            frame["source_path"] = str(csv_path)
            frames.append(frame)

    raw = pd.concat(frames, ignore_index=True)
    raw = raw[(raw["mode"] == "streaming") &
              (raw["sample_size"] == TARGET_N) &
              (raw["seed"] == TARGET_SEED)].copy()
    raw["rho"] = (raw["sample_size"] / raw["f0"]).astype(int)
    raw = raw[raw["rho"].isin(TARGET_RHO)].copy()
    raw = raw[(raw["f0_mean_t"] > 0) & (raw["f0_hat_mean_t"] > 0)].copy()
    return raw


def build_endpoint_summary(raw: pd.DataFrame) -> pd.DataFrame:
    end = (raw.sort_values("number_of_elements_processed")
              .groupby(["algorithm_label", "param_name", "param_value", "f0"], as_index=False)
              .tail(1)
              .copy())
    end["endpoint_mre"] = end["mean_relative_error"].astype(float)
    return end.sort_values(["algorithm_label", "rho", "param_value"])


def build_best_params_table(endpoint: pd.DataFrame) -> pd.DataFrame:
    compact = endpoint[["algorithm_label", "param_name", "param_value", "rho", "endpoint_mre"]].copy()
    idx = compact.groupby(["algorithm_label", "rho"])["endpoint_mre"].idxmin()
    best = compact.loc[idx].sort_values(["algorithm_label", "rho", "param_value"])
    return best.reset_index(drop=True)


def interpolate_point(group: pd.DataFrame, f0_star: float) -> tuple[float, float, float] | None:
    compact = (group.groupby("f0_mean_t", as_index=False)
                   .agg({
                       "number_of_elements_processed": "mean",
                       "f0_hat_mean_t": "mean",
                       "stddev": "mean",
                   })
                   .sort_values("f0_mean_t"))

    x = compact["f0_mean_t"].to_numpy(dtype=float)
    if len(x) < 2:
        return None
    if f0_star < x.min() or f0_star > x.max():
        return None

    t_vals = compact["number_of_elements_processed"].to_numpy(dtype=float)
    y_vals = compact["f0_hat_mean_t"].to_numpy(dtype=float)
    s_vals = compact["stddev"].to_numpy(dtype=float)

    t_interp = float(np.interp(f0_star, x, t_vals))
    y_interp = float(np.interp(f0_star, x, y_vals))
    s_interp = float(np.interp(f0_star, x, s_vals))
    rho_t = t_interp / float(f0_star)
    return rho_t, y_interp, s_interp


def build_type_b_points(raw: pd.DataFrame) -> pd.DataFrame:
    rows: list[dict[str, float | int | str]] = []
    for f0_star in TARGET_F0_STAR:
        for (algo, param_name, param_value, rho), group in raw.groupby(
            ["algorithm_label", "param_name", "param_value", "rho"]
        ):
            point = interpolate_point(group, float(f0_star))
            if point is None:
                continue
            rho_t, f0_hat, stddev = point
            rows.append({
                "algorithm_label": algo,
                "param_name": param_name,
                "param_value": int(param_value),
                "rho": int(rho),
                "f0_star": int(f0_star),
                "rho_t": float(rho_t),
                "f0_hat": float(f0_hat),
                "stddev": float(stddev),
            })

    if not rows:
        raise ValueError("Nessun punto Type B generato")
    return pd.DataFrame(rows)


def algorithm_palette(values: list[int], cmap_name: str = "viridis") -> dict[int, tuple[float, float, float, float]]:
    cmap = plt.get_cmap(cmap_name)
    scale = np.linspace(0.12, 0.92, num=len(values))
    return {value: cmap(pos) for value, pos in zip(values, scale)}


def log_band_from_mean_std(mean: np.ndarray, std: np.ndarray, eps: float = 1e-12) -> tuple[np.ndarray, np.ndarray]:
    clipped = np.maximum(mean, eps)
    cv = np.divide(std, clipped, out=np.zeros_like(mean), where=clipped > 0)
    sigma_log = np.sqrt(np.log1p(cv * cv))
    low = clipped * np.exp(-sigma_log)
    high = clipped * np.exp(sigma_log)
    return low, high


def plot_endpoint_heatmaps(endpoint: pd.DataFrame, out_path: Path) -> None:
    fig, axes = plt.subplots(2, 2, figsize=(13.5, 9.4), constrained_layout=True)
    axes = np.array(axes).reshape(-1)

    vmin = float(endpoint["endpoint_mre"].min())
    vmax = float(endpoint["endpoint_mre"].max())
    image = None

    for ax, spec in zip(axes, ALGORITHM_SPECS):
        algo = spec["label"]
        sub = endpoint[endpoint["algorithm_label"] == algo].copy()
        pivot = (sub.pivot_table(
            index="rho",
            columns="param_value",
            values="endpoint_mre",
            aggfunc="mean"
        ).reindex(index=TARGET_RHO))
        pivot = pivot.sort_index(axis=1)

        image = ax.imshow(pivot.to_numpy(dtype=float), aspect="auto", origin="lower", vmin=vmin, vmax=vmax, cmap="magma_r")
        ax.set_title(algo)
        ax.set_xlabel(spec["param_name"])
        ax.set_ylabel("rho")
        ax.set_xticks(np.arange(len(pivot.columns)))
        ax.set_xticklabels([str(int(v)) for v in pivot.columns])
        ax.set_yticks(np.arange(len(pivot.index)))
        ax.set_yticklabels([str(int(v)) for v in pivot.index])

        for row_idx, rho in enumerate(pivot.index):
            for col_idx, param_value in enumerate(pivot.columns):
                value = float(pivot.loc[rho, param_value])
                text_color = "white" if value > (vmin + 0.55 * (vmax - vmin)) else "black"
                ax.text(col_idx, row_idx, f"{value:.3f}", ha="center", va="center", color=text_color, fontsize=7)

    cbar = fig.colorbar(image, ax=axes.tolist(), shrink=0.88, pad=0.02)
    cbar.set_label("errore relativo medio all'endpoint")
    fig.suptitle("Sweep di precisione in streaming: heatmap dell'errore finale", fontsize=14)
    fig.savefig(out_path, dpi=180, bbox_inches="tight")
    plt.close(fig)


def plot_type_a_grid(raw: pd.DataFrame, out_path: Path, loglog: bool) -> None:
    fig, axes = plt.subplots(len(ALGORITHM_SPECS), len(TARGET_RHO), figsize=(17.5, 15.8), constrained_layout=True)
    axes = np.array(axes).reshape(len(ALGORITHM_SPECS), len(TARGET_RHO))

    for row_idx, spec in enumerate(ALGORITHM_SPECS):
        algo = spec["label"]
        algo_df = raw[raw["algorithm_label"] == algo].copy()
        param_values = sorted(algo_df["param_value"].unique().tolist())
        colors = algorithm_palette(param_values)

        for col_idx, rho in enumerate(TARGET_RHO):
            ax = axes[row_idx, col_idx]
            sub = algo_df[algo_df["rho"] == rho].copy()
            x_min = float(sub["f0_mean_t"].min())
            x_max = float(sub["f0_mean_t"].max())
            diag_x = np.array([x_min, x_max], dtype=float)
            ax.plot(diag_x, diag_x, color="black", linestyle="--", linewidth=1.1, label="riferimento")

            for param_value in param_values:
                part = sub[sub["param_value"] == param_value].sort_values("f0_mean_t")
                x = part["f0_mean_t"].to_numpy(dtype=float)
                y = part["f0_hat_mean_t"].to_numpy(dtype=float)
                s = part["stddev"].to_numpy(dtype=float)
                label = f"{spec['param_name']}={param_value}"
                color = colors[param_value]
                ax.plot(x, y, color=color, linewidth=1.7, label=label)
                if loglog:
                    low, high = log_band_from_mean_std(y, s)
                    ax.fill_between(x, low, high, color=color, alpha=0.12)
                else:
                    ax.fill_between(x, np.maximum(y - s, 1e-12), y + s, color=color, alpha=0.12)

            if row_idx == 0:
                ax.set_title(f"rho = {rho}")
            if col_idx == 0:
                ax.set_ylabel(f"{algo}\nF0 stimata")
            else:
                ax.set_ylabel("F0 stimata")
            if row_idx == len(ALGORITHM_SPECS) - 1:
                ax.set_xlabel("F0 reale")

            if loglog:
                ax.set_xscale("log")
                ax.set_yscale("log")

            if col_idx == 0:
                ax.legend(loc="best", fontsize=6.5, frameon=True)

    title = "Sweep di precisione in streaming - confronto tipo A"
    if loglog:
        title += " (log-log)"
    else:
        title += " (lineare)"
    fig.suptitle(title, fontsize=14)
    fig.savefig(out_path, dpi=180, bbox_inches="tight")
    plt.close(fig)


def plot_type_b_grid(raw: pd.DataFrame, out_path: Path, loglog: bool) -> None:
    points = build_type_b_points(raw)
    fig, axes = plt.subplots(len(ALGORITHM_SPECS), len(TARGET_RHO), figsize=(17.5, 15.8), constrained_layout=True)
    axes = np.array(axes).reshape(len(ALGORITHM_SPECS), len(TARGET_RHO))

    for row_idx, spec in enumerate(ALGORITHM_SPECS):
        algo = spec["label"]
        algo_df = points[points["algorithm_label"] == algo].copy()
        param_values = sorted(algo_df["param_value"].unique().tolist())
        colors = algorithm_palette(param_values)

        for col_idx, rho in enumerate(TARGET_RHO):
            ax = axes[row_idx, col_idx]
            sub = algo_df[algo_df["rho"] == rho].copy()

            for param_value in param_values:
                part = sub[sub["param_value"] == param_value].sort_values("f0_star")
                x = part["f0_star"].to_numpy(dtype=float)
                y = part["f0_hat"].to_numpy(dtype=float)
                s = part["stddev"].to_numpy(dtype=float)
                label = f"{spec['param_name']}={param_value}"
                color = colors[param_value]
                ax.plot(x, y, marker="o", markersize=3.6, linewidth=1.5, color=color, label=label)
                if loglog:
                    low, high = log_band_from_mean_std(y, s)
                    ax.fill_between(x, low, high, color=color, alpha=0.12)
                else:
                    ax.fill_between(x, y - s, y + s, color=color, alpha=0.12)

            truth = np.array(TARGET_F0_STAR, dtype=float)
            ax.plot(truth, truth, linestyle="--", color="black", linewidth=1.0, label="riferimento")

            if row_idx == 0:
                ax.set_title(f"rho finale = {rho}")
            if col_idx == 0:
                ax.set_ylabel(f"{algo}\nF0 stimata")
            else:
                ax.set_ylabel("F0 stimata")
            if row_idx == len(ALGORITHM_SPECS) - 1:
                ax.set_xlabel("F0*")

            ax.set_xscale("log")
            if loglog:
                ax.set_yscale("log")

            if col_idx == 0:
                ax.legend(loc="best", fontsize=6.5, frameon=True)

    title = "Sweep di precisione in streaming - confronto tipo B"
    if loglog:
        title += " (log-log)"
    else:
        title += " (lineare)"
    fig.suptitle(title, fontsize=14)
    fig.savefig(out_path, dpi=180, bbox_inches="tight")
    plt.close(fig)


def export_all(repo_root: Path) -> tuple[pd.DataFrame, pd.DataFrame]:
    plt.style.use("seaborn-v0_8-whitegrid")

    raw = load_streaming_precision_data(repo_root)
    endpoint = build_endpoint_summary(raw)
    best = build_best_params_table(endpoint)

    results_out = repo_root / "results"
    figures_out = repo_root / "thesis" / "figures" / "results"
    figures_out.mkdir(parents=True, exist_ok=True)

    endpoint[["algorithm_label", "param_name", "param_value", "rho", "endpoint_mre"]].to_csv(
        results_out / "streaming_precision_sweep_summary_endpoint.csv", index=False
    )
    best.to_csv(results_out / "streaming_precision_sweep_best_params.csv", index=False)

    plot_endpoint_heatmaps(endpoint, figures_out / "streaming_precision_sweep_endpoint_heatmaps.png")
    plot_type_a_grid(raw, figures_out / "streaming_precision_sweep_typeA_grid_linear.png", loglog=False)
    plot_type_b_grid(raw, figures_out / "streaming_precision_sweep_typeB_grid_linear.png", loglog=False)

    return endpoint, best


def main() -> None:
    repo_root = find_repo_root(Path(__file__).resolve())
    endpoint, best = export_all(repo_root)
    print("endpoint rows:", len(endpoint))
    print(best.to_string(index=False))


if __name__ == "__main__":
    main()
