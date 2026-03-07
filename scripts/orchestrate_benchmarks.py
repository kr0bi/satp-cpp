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
DEFAULT_HASH_FUNCTION = "splitmix64"
DEFAULT_ALGORITHMS = "all"

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


def parse_csv_strings(raw: str) -> list[str]:
    values = [x.strip() for x in raw.split(",") if x.strip()]
    if not values:
        raise ValueError("Empty string list")
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
                   hash_function: str,
                   commands: list[str],
                   run_label: str) -> None:
    payload = "\n".join([
        f"set datasetPath {dataset_path}",
        f"set resultsNamespace {results_namespace}",
        f"set hashFunction {hash_function}",
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


def format_results_namespace(template: str,
                             base: str,
                             n: int,
                             d: int,
                             p: int,
                             seed: int) -> str:
    return template.format(base=base, n=n, d=d, p=p, seed=seed)


def append_run_modes(commands: list[str],
                     algo: str,
                     run_streaming: bool,
                     run_merge: bool) -> None:
    if run_streaming:
        commands.append(f"runstream {algo}")
    if run_merge:
        commands.append(f"runmerge {algo}")


def build_standard_commands(k: int,
                            l: int,
                            l_log: int,
                            run_streaming: bool,
                            run_merge: bool) -> list[str]:
    commands = [
        f"set k {k}",
        f"set l {l}",
        f"set lLog {l_log}",
    ]
    if run_streaming:
        commands.append("runstream all")
    if run_merge:
        commands.append("runmerge all")
    return commands


def build_selected_commands(algorithms: list[str],
                            k_values: list[int],
                            l_values: list[int],
                            l_log_values: list[int],
                            run_streaming: bool,
                            run_merge: bool) -> list[str]:
    supported = {"hllpp", "hll", "ll", "pc"}
    unknown = [algo for algo in algorithms if algo not in supported]
    if unknown:
        raise ValueError(f"Unsupported algorithm ids: {', '.join(unknown)}")

    commands: list[str] = []
    default_k = k_values[0]
    default_l = l_values[0]
    default_l_log = l_log_values[0]

    for algo in algorithms:
        if algo == "hllpp":
            for k in k_values:
                commands.extend([
                    f"set k {k}",
                    f"set l {default_l}",
                    f"set lLog {default_l_log}",
                ])
                append_run_modes(commands, "hllpp", run_streaming, run_merge)
            continue

        if algo == "hll":
            for k in k_values:
                for l_log in l_log_values:
                    commands.extend([
                        f"set k {k}",
                        f"set l {default_l}",
                        f"set lLog {l_log}",
                    ])
                    append_run_modes(commands, "hll", run_streaming, run_merge)
            continue

        if algo == "ll":
            for k in k_values:
                for l_log in l_log_values:
                    commands.extend([
                        f"set k {k}",
                        f"set l {default_l}",
                        f"set lLog {l_log}",
                    ])
                    append_run_modes(commands, "ll", run_streaming, run_merge)
            continue

        if algo == "pc":
            for l in l_values:
                commands.extend([
                    f"set k {default_k}",
                    f"set l {l}",
                    f"set lLog {default_l_log}",
                ])
                append_run_modes(commands, "pc", run_streaming, run_merge)

    return commands


def build_full_commands(base_l: int,
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
        append_run_modes(commands, "hllpp", run_streaming, run_merge)

    # HLL: k in [4,16], L fixed at 32
    for k in HLL_K_DOMAIN:
        commands.extend([
            f"set k {k}",
            f"set l {base_l}",
            f"set lLog {PAPER_LLOG}",
        ])
        append_run_modes(commands, "hll", run_streaming, run_merge)

    # LogLog: k in [4,16], L fixed at 32
    for k in LL_K_DOMAIN:
        commands.extend([
            f"set k {k}",
            f"set l {base_l}",
            f"set lLog {PAPER_LLOG}",
        ])
        append_run_modes(commands, "ll", run_streaming, run_merge)

    # Probabilistic Counting: L in [1,31]
    for l in PC_L_DOMAIN:
        commands.extend([
            f"set k {LL_K_DOMAIN[-1]}",
            f"set l {l}",
            f"set lLog {PAPER_LLOG}",
        ])
        append_run_modes(commands, "pc", run_streaming, run_merge)

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
            "Generate dataset matrix and run SATP benchmarks (streaming/merge). "
            "Results are written directly to results/<namespace>/<mode>/<algorithm>/<hash>/<params>/."
        )
    )
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--main-bin", type=Path, default=Path("build/main"))
    parser.add_argument("--generator", type=Path, default=Path("scripts/generate_prefix_constant_rho_dataset_bin.py"))
    parser.add_argument("--dataset-dir", type=Path, default=Path("datasets/prefix_constant_rho"))
    parser.add_argument("--results-namespace", type=str, default=DEFAULT_RESULTS_NAMESPACE,
                        help="Namespace under results/ for orchestrated outputs")
    parser.add_argument("--results-namespace-template", type=str, default="{base}",
                        help="Template for per-job namespace. Placeholders: {base}, {n}, {d}, {p}, {seed}")
    parser.add_argument("--hash-function", type=str, default=DEFAULT_HASH_FUNCTION,
                        help="Hash function name passed to SATP CLI (default: splitmix64)")
    parser.add_argument("--hash-functions", default=None,
                        help="Optional CSV of hash functions to sweep instead of a single --hash-function")
    parser.add_argument("--algorithms", default=DEFAULT_ALGORITHMS,
                        help="CSV of algorithm ids to run: all,hllpp,hll,ll,pc (default: all)")
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
    parser.add_argument("--skip-streaming", action="store_true",
                        help="Skip streaming mode (runstream all)")
    parser.add_argument("--skip-merge", action="store_true",
                        help="Skip merge mode (runmerge all)")
    parser.add_argument("--full", action="store_true",
                        help="Run all valid parameter domains per algorithm")
    parser.add_argument("--k", type=int, default=16,
                        help="k parameter for HLL/HLL++/LogLog")
    parser.add_argument("--k-values", default=None,
                        help="Optional CSV of k values to sweep for HLL++/HLL/LogLog")
    parser.add_argument("--l", type=int, default=16,
                        help="L parameter for ProbabilisticCounting")
    parser.add_argument("--l-values", default=None,
                        help="Optional CSV of L values to sweep for ProbabilisticCounting")
    parser.add_argument("--l-log", type=int, default=32,
                        help="L parameter for LogLog/HLL")
    parser.add_argument("--l-log-values", default=None,
                        help="Optional CSV of LLog values to sweep for LogLog/HLL")
    parser.add_argument("--clean-results", action="store_true",
                        help="Delete repo_root/results before running")
    parser.add_argument("--clean-target-results", action="store_true",
                        help="Delete only the result namespaces targeted by this run before executing")
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

    if args.skip_streaming and args.skip_merge:
        raise ValueError("All benchmark modes are skipped: nothing to run")

    selected_algorithms = parse_csv_strings(args.algorithms)
    hash_functions = parse_csv_strings(args.hash_functions) if args.hash_functions else [args.hash_function]
    k_values = parse_csv_ints(args.k_values) if args.k_values else [args.k]
    l_values = parse_csv_ints(args.l_values) if args.l_values else [args.l]
    l_log_values = parse_csv_ints(args.l_log_values) if args.l_log_values else [args.l_log]

    if "all" in selected_algorithms and len(selected_algorithms) > 1:
        raise ValueError("'all' cannot be combined with other algorithm ids")
    if args.full and selected_algorithms != ["all"]:
        raise ValueError("--full currently requires --algorithms all")

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

    target_namespaces = sorted({
        format_results_namespace(
            template=args.results_namespace_template,
            base=args.results_namespace,
            n=n,
            d=d,
            p=args.p,
            seed=seed,
        )
        for (n, d, seed) in jobs
    })

    if args.clean_target_results:
        for namespace in target_namespaces:
            namespace_dir = repo_root / "results" / namespace
            if namespace_dir.exists():
                print(f"[clean-target] removing {namespace_dir}")
                shutil.rmtree(namespace_dir)

    total = len(jobs)
    print(f"[plan] jobs={total} (n x d x seed), p={args.p}")
    print(f"[datasets] {dataset_dir}")
    print(f"[results] {repo_root / 'results' / args.results_namespace}")
    print(f"[results-template] {args.results_namespace_template}")
    print(
        f"[params] hashFunctions={','.join(hash_functions)} algorithms={','.join(selected_algorithms)} "
        f"k={','.join(str(v) for v in k_values)} l={','.join(str(v) for v in l_values)} "
        f"lLog={','.join(str(v) for v in l_log_values)} "
        f"streaming={not args.skip_streaming} merge={not args.skip_merge} full={args.full}"
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

        resolved_namespace = format_results_namespace(
            template=args.results_namespace_template,
            base=args.results_namespace,
            n=n,
            d=d,
            p=args.p,
            seed=seed,
        )

        if args.full:
            commands = build_full_commands(
                base_l=args.l,
                run_streaming=not args.skip_streaming,
                run_merge=not args.skip_merge,
            )
            run_label = "full-domains"
        else:
            if selected_algorithms == ["all"]:
                commands = build_standard_commands(
                    k=args.k,
                    l=args.l,
                    l_log=args.l_log,
                    run_streaming=not args.skip_streaming,
                    run_merge=not args.skip_merge,
                )
                run_label = "single-params"
            else:
                commands = build_selected_commands(
                    algorithms=selected_algorithms,
                    k_values=k_values,
                    l_values=l_values,
                    l_log_values=l_log_values,
                    run_streaming=not args.skip_streaming,
                    run_merge=not args.skip_merge,
                )
                run_label = "selected-sweep"

        for hash_function in hash_functions:
            run_benchmarks(
                main_bin=main_bin,
                dataset_path=dataset_path,
                results_namespace=resolved_namespace,
                hash_function=hash_function,
                commands=commands,
                run_label=f"{run_label}:{hash_function}",
            )

    print("\n[done] orchestration completed")


if __name__ == "__main__":
    main()
