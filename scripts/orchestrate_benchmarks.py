#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


DEFAULT_N_VALUES = "100,1000,10000,100000,1000000,10000000"
DEFAULT_D_RATIOS = "0.01,0.1,0.5,1.0"
DEFAULT_SEEDS = "21041998,42,137357,10032018,29042026"
DEFAULT_PARTITIONS = 50
DEFAULT_RESULTS_NAMESPACE = "prefix_constant_rho"

PC_L_DOMAIN = list(range(1, 32))        # ProbabilisticCounting: L in [1,31]
LL_K_DOMAIN = list(range(4, 17))        # LogLog: k in [4,16], L=32
HLL_K_DOMAIN = list(range(4, 17))       # HyperLogLog: k in [4,16], L=32
HLLPP_K_DOMAIN = list(range(4, 19))     # HyperLogLog++: p(=k) in [4,18]
PAPER_LLOG = 32


def parse_csv_ints(raw: str) -> list[int]:
    values = [int(x.strip()) for x in raw.split(",") if x.strip()]
    if not values:
        raise ValueError("Empty integer list")
    return values


def parse_csv_floats(raw: str) -> list[float]:
    values = [float(x.strip()) for x in raw.split(",") if x.strip()]
    if not values:
        raise ValueError("Empty float list")
    return values


def dataset_name(n: int, d: int, p: int, seed: int) -> str:
    return f"dataset_n_{n}_d_{d}_p_{p}_s_{seed}.bin"


def run_cmd(cmd: list[str], cwd: Path) -> None:
    print("$", " ".join(cmd))
    subprocess.run(cmd, cwd=cwd, check=True)


def ensure_dataset(repo_root: Path,
                   dataset_dir: Path,
                   generator: Path,
                   n: int,
                   d: int,
                   p: int,
                   seed: int,
                   workers: int | None,
                   no_progress: bool) -> Path:
    out_path = dataset_dir / dataset_name(n, d, p, seed)
    if out_path.exists():
        print(f"[dataset] skip existing: {out_path}")
        return out_path

    cmd = [
        sys.executable,
        str(generator),
        "--output-dir", str(dataset_dir),
        "--n", str(n),
        "--d", str(d),
        "--p", str(p),
        "--seed", str(seed),
    ]
    if workers is not None:
        cmd.extend(["--workers", str(workers)])
    if no_progress:
        cmd.append("--no-progress")
    run_cmd(cmd, cwd=repo_root)
    return out_path


def run_benchmarks(main_bin: Path,
                   dataset_path: Path,
                   results_namespace: str,
                   commands: list[str],
                   run_label: str) -> None:
    payload = "\n".join([
        f"set datasetPath {dataset_path}",
        f"set resultsNamespace {results_namespace}",
        *commands,
        "quit"
    ]) + "\n"

    print(f"[bench] dataset={dataset_path} mode={run_label}")
    subprocess.run(
        [str(main_bin)],
        input=payload,
        text=True,
        check=True,
    )


def append_run_modes(commands: list[str],
                     algo: str,
                     run_oneshot: bool,
                     run_streaming: bool,
                     run_merge: bool) -> None:
    if run_oneshot:
        commands.append(f"run {algo}")
    if run_streaming:
        commands.append(f"runstream {algo}")
    if run_merge:
        commands.append(f"runmerge {algo}")


def build_standard_commands(k: int,
                            l: int,
                            l_log: int,
                            run_oneshot: bool,
                            run_streaming: bool,
                            run_merge: bool) -> list[str]:
    commands = [
        f"set k {k}",
        f"set l {l}",
        f"set lLog {l_log}",
    ]
    if run_oneshot:
        commands.append("run all")
    if run_streaming:
        commands.append("runstream all")
    if run_merge:
        commands.append("runmerge all")
    return commands


def build_full_commands(base_l: int,
                        run_oneshot: bool,
                        run_streaming: bool,
                        run_merge: bool) -> list[str]:
    commands: list[str] = []

    # HLL++: p(=k) in [4,18]
    for k in HLLPP_K_DOMAIN:
        commands.extend([
            f"set k {k}",
            f"set l {base_l}",
            f"set lLog {PAPER_LLOG}",
        ])
        append_run_modes(commands, "hllpp", run_oneshot, run_streaming, run_merge)

    # HLL: k in [4,16], L fixed at 32
    for k in HLL_K_DOMAIN:
        commands.extend([
            f"set k {k}",
            f"set l {base_l}",
            f"set lLog {PAPER_LLOG}",
        ])
        append_run_modes(commands, "hll", run_oneshot, run_streaming, run_merge)

    # LogLog: k in [4,16], L fixed at 32
    for k in LL_K_DOMAIN:
        commands.extend([
            f"set k {k}",
            f"set l {base_l}",
            f"set lLog {PAPER_LLOG}",
        ])
        append_run_modes(commands, "ll", run_oneshot, run_streaming, run_merge)

    # Probabilistic Counting: L in [1,31]
    for l in PC_L_DOMAIN:
        commands.extend([
            f"set k {LL_K_DOMAIN[-1]}",
            f"set l {l}",
            f"set lLog {PAPER_LLOG}",
        ])
        append_run_modes(commands, "pc", run_oneshot, run_streaming, run_merge)

    return commands


def compute_distincts(n: int, ratios: list[float]) -> list[int]:
    out = []
    for r in ratios:
        if r <= 0.0 or r > 1.0:
            raise ValueError(f"d ratio out of range (0,1]: {r}")
        d = int(round(n * r))
        d = max(1, min(d, n))
        out.append(d)
    return sorted(set(out))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate dataset matrix and run SATP benchmarks (oneshot/streaming/merge). "
            "Results are written directly to results/<namespace>/<mode>/<algorithm>/<params>/."
        )
    )
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--main-bin", type=Path, default=Path("build/main"))
    parser.add_argument("--generator", type=Path, default=Path("scripts/generate_prefix_constant_rho_dataset_bin.py"))
    parser.add_argument("--dataset-dir", type=Path, default=Path("datasets/prefix_constant_rho"))
    parser.add_argument("--results-namespace", type=str, default=DEFAULT_RESULTS_NAMESPACE,
                        help="Namespace under results/ for orchestrated outputs")
    parser.add_argument("--n-values", default=DEFAULT_N_VALUES)
    parser.add_argument("--d-ratios", default=DEFAULT_D_RATIOS)
    parser.add_argument("--seeds", default=DEFAULT_SEEDS)
    parser.add_argument("--p", type=int, default=DEFAULT_PARTITIONS)
    parser.add_argument("--workers", type=int, default=None,
                        help="Workers passed to dataset generator")
    parser.add_argument("--no-progress", action="store_true",
                        help="Disable dataset generation progress output")
    parser.add_argument("--skip-generate", action="store_true",
                        help="Use only already-existing datasets")
    parser.add_argument("--skip-oneshot", action="store_true",
                        help="Skip normal mode (run all)")
    parser.add_argument("--skip-streaming", action="store_true",
                        help="Skip streaming mode (runstream all)")
    parser.add_argument("--skip-merge", action="store_true",
                        help="Skip merge mode (runmerge all)")
    parser.add_argument("--full", action="store_true",
                        help="Run all valid parameter domains per algorithm")
    parser.add_argument("--k", type=int, default=16,
                        help="k parameter for HLL/HLL++/LogLog")
    parser.add_argument("--l", type=int, default=16,
                        help="L parameter for ProbabilisticCounting")
    parser.add_argument("--l-log", type=int, default=32,
                        help="L parameter for LogLog/HLL")
    parser.add_argument("--clean-results", action="store_true",
                        help="Delete repo_root/results before running")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    repo_root = args.repo_root.resolve()
    main_bin = (repo_root / args.main_bin).resolve()
    generator = (repo_root / args.generator).resolve()
    dataset_dir = (repo_root / args.dataset_dir).resolve()
    dataset_dir.mkdir(parents=True, exist_ok=True)

    if not main_bin.exists():
        raise FileNotFoundError(f"Main binary not found: {main_bin}")
    if not generator.exists():
        raise FileNotFoundError(f"Generator script not found: {generator}")

    if args.skip_oneshot and args.skip_streaming and args.skip_merge:
        raise ValueError("All benchmark modes are skipped: nothing to run")

    if args.clean_results:
        results_dir = repo_root / "results"
        if results_dir.exists():
            print(f"[clean] removing {results_dir}")
            shutil.rmtree(results_dir)

    n_values = parse_csv_ints(args.n_values)
    d_ratios = parse_csv_floats(args.d_ratios)
    seeds = parse_csv_ints(args.seeds)

    jobs: list[tuple[int, int, int]] = []
    for n in n_values:
        for d in compute_distincts(n, d_ratios):
            for seed in seeds:
                jobs.append((n, d, seed))

    total = len(jobs)
    print(f"[plan] jobs={total} (n x d x seed), p={args.p}")
    print(f"[datasets] {dataset_dir}")
    print(f"[results] {repo_root / 'results' / args.results_namespace}")
    print(
        f"[params] k={args.k} l={args.l} lLog={args.l_log} "
        f"oneshot={not args.skip_oneshot} streaming={not args.skip_streaming} merge={not args.skip_merge} "
        f"full={args.full}"
    )
    if args.full:
        print(
            "[domains] "
            f"HLL++ k={HLLPP_K_DOMAIN[0]}..{HLLPP_K_DOMAIN[-1]}, "
            f"HLL k={HLL_K_DOMAIN[0]}..{HLL_K_DOMAIN[-1]} L={PAPER_LLOG}, "
            f"LogLog k={LL_K_DOMAIN[0]}..{LL_K_DOMAIN[-1]} L={PAPER_LLOG}, "
            f"PC L={PC_L_DOMAIN[0]}..{PC_L_DOMAIN[-1]}"
        )

    for idx, (n, d, seed) in enumerate(jobs, start=1):
        print(f"\n[{idx}/{total}] n={n} d={d} p={args.p} seed={seed}")
        dataset_path = dataset_dir / dataset_name(n, d, args.p, seed)
        if not args.skip_generate:
            dataset_path = ensure_dataset(
                repo_root=repo_root,
                dataset_dir=dataset_dir,
                generator=generator,
                n=n,
                d=d,
                p=args.p,
                seed=seed,
                workers=args.workers,
                no_progress=args.no_progress,
            )
        elif not dataset_path.exists():
            raise FileNotFoundError(f"Missing dataset (use without --skip-generate): {dataset_path}")

        if args.full:
            commands = build_full_commands(
                base_l=args.l,
                run_oneshot=not args.skip_oneshot,
                run_streaming=not args.skip_streaming,
                run_merge=not args.skip_merge,
            )
            run_label = "full-domains"
        else:
            commands = build_standard_commands(
                k=args.k,
                l=args.l,
                l_log=args.l_log,
                run_oneshot=not args.skip_oneshot,
                run_streaming=not args.skip_streaming,
                run_merge=not args.skip_merge,
            )
            run_label = "single-params"

        run_benchmarks(
            main_bin=main_bin,
            dataset_path=dataset_path,
            results_namespace=args.results_namespace,
            commands=commands,
            run_label=run_label,
        )

    print("\n[done] orchestration completed")


if __name__ == "__main__":
    main()
