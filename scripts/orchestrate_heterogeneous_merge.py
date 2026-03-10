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
DEFAULT_RESULTS_NAMESPACE = "merge_hllpp_heterogeneous"
DEFAULT_HASH_FUNCTION = "splitmix64"
DEFAULT_HASH_SEEDS = "dataset"
DEFAULT_K_VALUES = "14"
DEFAULT_MERGE_STRATEGIES = "direct"


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


def run_cmd(cmd: list[str], cwd: Path, input_text: str | None = None) -> None:
    print("$", " ".join(cmd))
    subprocess.run(cmd, cwd=cwd, input=input_text, text=True, check=True)


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


def compute_distincts(n: int, ratios: list[float]) -> list[int]:
    out: list[int] = []
    for ratio in ratios:
        if ratio <= 0.0 or ratio > 1.0:
            raise ValueError(f"d ratio out of range (0,1]: {ratio}")
        d = int(round(n * ratio))
        d = max(1, min(d, n))
        out.append(d)
    return sorted(set(out))


def format_results_namespace(template: str,
                             base: str,
                             n: int,
                             d: int,
                             p: int,
                             seed: int) -> str:
    return template.format(base=base, n=n, d=d, p=p, seed=seed)


def resolve_seed_token(token: str, dataset_seed: int) -> int:
    if token == "dataset":
        return dataset_seed
    return int(token)


def classify_hllpp_case(left_hash: str,
                        left_seed: int,
                        left_k: int,
                        right_hash: str,
                        right_seed: int,
                        right_k: int) -> str:
    same_hash_semantics = left_hash == right_hash and left_seed == right_seed
    same_k = left_k == right_k
    if same_hash_semantics and same_k:
        return "valid"
    if same_hash_semantics:
        return "recoverable"
    return "invalid"


def strategy_allowed(strategy: str, validity: str) -> tuple[bool, str]:
    if strategy == "reject":
        return True, ""
    if strategy == "direct":
        return validity == "valid", "direct richiede un caso valid"
    if strategy == "unsafe_naive_merge":
        return validity != "valid", "unsafe_naive_merge richiede un caso non-valid"
    if strategy == "reduce_then_merge":
        return validity == "recoverable", "reduce_then_merge richiede un caso recoverable"
    return False, f"strategia non supportata: {strategy}"


def run_heterogeneous_benchmark(repo_root: Path,
                                main_bin: Path,
                                dataset_path: Path,
                                results_namespace: str,
                                left_hash_function: str,
                                left_hash_seed: int,
                                left_k: int,
                                right_hash_function: str,
                                right_hash_seed: int,
                                right_k: int,
                                merge_strategy: str) -> None:
    payload = "\n".join([
        f"set datasetPath {dataset_path}",
        f"set resultsNamespace {results_namespace}",
        f"set hashFunction {left_hash_function}",
        f"set leftHashFunction {left_hash_function}",
        f"set rightHashFunction {right_hash_function}",
        f"set leftHashSeed {left_hash_seed}",
        f"set rightHashSeed {right_hash_seed}",
        f"set leftK {left_k}",
        f"set rightK {right_k}",
        f"set mergeStrategy {merge_strategy}",
        "runmergehet hllpp",
        "quit"
    ]) + "\n"

    print(
        "[bench] "
        f"dataset={dataset_path} left=({left_hash_function},seed={left_hash_seed},k={left_k}) "
        f"right=({right_hash_function},seed={right_hash_seed},k={right_k}) "
        f"strategy={merge_strategy}"
    )
    run_cmd([str(main_bin)], cwd=repo_root, input_text=payload)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate dataset matrix and run heterogeneous HLL++ merge benchmarks. "
            "Results are written to results/<namespace>/merge_heterogeneous/hllpp/..."
        )
    )
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--main-bin", type=Path, default=Path("build/main"))
    parser.add_argument("--generator", type=Path, default=Path("scripts/generate_prefix_constant_rho_dataset_bin.py"))
    parser.add_argument("--dataset-dir", type=Path, default=Path("datasets/prefix_constant_rho"))
    parser.add_argument("--results-namespace", type=str, default=DEFAULT_RESULTS_NAMESPACE)
    parser.add_argument("--results-namespace-template", type=str, default="{base}")
    parser.add_argument("--n-values", default=DEFAULT_N_VALUES)
    parser.add_argument("--d-ratios", default=DEFAULT_D_RATIOS)
    parser.add_argument("--seeds", default=DEFAULT_SEEDS)
    parser.add_argument("--p", type=int, default=DEFAULT_PARTITIONS)
    parser.add_argument("--workers", type=int, default=None)
    parser.add_argument("--no-progress", action="store_true")
    parser.add_argument("--skip-generate", action="store_true")
    parser.add_argument("--left-hash-functions", default=DEFAULT_HASH_FUNCTION)
    parser.add_argument("--right-hash-functions", default=DEFAULT_HASH_FUNCTION)
    parser.add_argument("--left-hash-seeds", default=DEFAULT_HASH_SEEDS,
                        help="CSV of seeds or the special token 'dataset'")
    parser.add_argument("--right-hash-seeds", default=DEFAULT_HASH_SEEDS,
                        help="CSV of seeds or the special token 'dataset'")
    parser.add_argument("--left-k-values", default=DEFAULT_K_VALUES)
    parser.add_argument("--right-k-values", default=DEFAULT_K_VALUES)
    parser.add_argument("--merge-strategies", default=DEFAULT_MERGE_STRATEGIES)
    parser.add_argument("--clean-results", action="store_true")
    parser.add_argument("--clean-target-results", action="store_true")
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

    left_hash_functions = parse_csv_strings(args.left_hash_functions)
    right_hash_functions = parse_csv_strings(args.right_hash_functions)
    left_hash_seed_tokens = parse_csv_strings(args.left_hash_seeds)
    right_hash_seed_tokens = parse_csv_strings(args.right_hash_seeds)
    left_k_values = parse_csv_ints(args.left_k_values)
    right_k_values = parse_csv_ints(args.right_k_values)
    merge_strategies = parse_csv_strings(args.merge_strategies)
    n_values = parse_csv_ints(args.n_values)
    d_ratios = parse_csv_floats(args.d_ratios)
    seeds = parse_csv_ints(args.seeds)

    if args.clean_results:
        results_dir = repo_root / "results"
        if results_dir.exists():
            print(f"[clean] removing {results_dir}")
            shutil.rmtree(results_dir)

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
        "[params] "
        f"leftHashFunctions={','.join(left_hash_functions)} "
        f"rightHashFunctions={','.join(right_hash_functions)} "
        f"leftHashSeeds={','.join(left_hash_seed_tokens)} "
        f"rightHashSeeds={','.join(right_hash_seed_tokens)} "
        f"leftK={','.join(str(v) for v in left_k_values)} "
        f"rightK={','.join(str(v) for v in right_k_values)} "
        f"mergeStrategies={','.join(merge_strategies)}"
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

        for left_hash in left_hash_functions:
            for right_hash in right_hash_functions:
                for left_seed_token in left_hash_seed_tokens:
                    left_hash_seed = resolve_seed_token(left_seed_token, seed)
                    for right_seed_token in right_hash_seed_tokens:
                        right_hash_seed = resolve_seed_token(right_seed_token, seed)
                        for left_k in left_k_values:
                            for right_k in right_k_values:
                                validity = classify_hllpp_case(
                                    left_hash,
                                    left_hash_seed,
                                    left_k,
                                    right_hash,
                                    right_hash_seed,
                                    right_k,
                                )
                                for strategy in merge_strategies:
                                    allowed, reason = strategy_allowed(strategy, validity)
                                    if not allowed:
                                        print(
                                            "[skip] "
                                            f"left=({left_hash},seed={left_hash_seed},k={left_k}) "
                                            f"right=({right_hash},seed={right_hash_seed},k={right_k}) "
                                            f"strategy={strategy} validity={validity}: {reason}"
                                        )
                                        continue

                                    run_heterogeneous_benchmark(
                                        repo_root=repo_root,
                                        main_bin=main_bin,
                                        dataset_path=dataset_path,
                                        results_namespace=resolved_namespace,
                                        left_hash_function=left_hash,
                                        left_hash_seed=left_hash_seed,
                                        left_k=left_k,
                                        right_hash_function=right_hash,
                                        right_hash_seed=right_hash_seed,
                                        right_k=right_k,
                                        merge_strategy=strategy,
                                    )

    print("\n[done] heterogeneous merge orchestration completed")


if __name__ == "__main__":
    main()
