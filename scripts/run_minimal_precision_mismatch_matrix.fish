#!/usr/bin/env fish

set -g SCRIPT_DIR (cd (dirname (status filename)); and pwd)
set -g REPO_ROOT (cd "$SCRIPT_DIR/.."; and pwd)

set -g PYTHON_BIN "$REPO_ROOT/.venv/bin/python"
if not test -x "$PYTHON_BIN"
    set -g PYTHON_BIN python3
end

set -l skip_generate 1
set -l clean_target 1
set -l build_first 1

function print_help
    echo "Usage: "(status filename)" [--generate] [--no-clean] [--skip-build] [--help]"
    echo
    echo "Runs the minimal HLL++ precision-mismatch matrix with reject, unsafe_naive_merge and reduce_then_merge."
    echo
    echo "Options:"
    echo "  --generate    Regenerate datasets instead of using --skip-generate"
    echo "  --no-clean    Keep existing target namespaces under results/"
    echo "  --skip-build  Skip 'cmake --build build -j 8'"
    echo "  --help        Show this help"
end

for arg in $argv
    switch $arg
        case --generate
            set skip_generate 0
        case --no-clean
            set clean_target 0
        case --skip-build
            set build_first 0
        case --help -h
            print_help
            exit 0
        case '*'
            echo "Unknown option: $arg" >&2
            print_help >&2
            exit 1
    end
end

if test $build_first -eq 1
    echo "[build] cmake -S $REPO_ROOT -B $REPO_ROOT/build"
    cmake -S "$REPO_ROOT" -B "$REPO_ROOT/build"
    or exit 1

    echo "[build] cmake --build $REPO_ROOT/build -j 8"
    cmake --build "$REPO_ROOT/build" -j 8
    or exit 1
end

set -g MATRIX_COMMON_ARGS \
    --repo-root "$REPO_ROOT" \
    --main-bin build/main \
    --generator scripts/generate_prefix_constant_rho_dataset_bin.py \
    --dataset-dir datasets/prefix_constant_rho \
    --results-namespace-template '{base}_d_{d}' \
    --n-values 10000000 \
    --d-ratios 0.01,0.1,1.0 \
    --seeds 21041998 \
    --p 50 \
    --left-hash-functions xxhash64 \
    --right-hash-functions xxhash64 \
    --left-hash-seeds dataset \
    --right-hash-seeds dataset

if test $skip_generate -eq 1
    set -a MATRIX_COMMON_ARGS --skip-generate
end

if test $clean_target -eq 1
    set -a MATRIX_COMMON_ARGS --clean-target-results
end

function run_block --argument-names namespace
    set -e argv[1]

    echo
    echo "[matrix] $namespace"
    "$PYTHON_BIN" "$REPO_ROOT/scripts/orchestrate_heterogeneous_merge.py" \
        $MATRIX_COMMON_ARGS \
        --results-namespace $namespace \
        $argv
    or exit 1
end

run_block merge_hpp_precision_mismatch_min_10_14 \
    --left-k-values 10 \
    --right-k-values 14 \
    --merge-strategies reject,unsafe_naive_merge,reduce_then_merge

run_block merge_hpp_precision_mismatch_min_14_10 \
    --left-k-values 14 \
    --right-k-values 10 \
    --merge-strategies reject,unsafe_naive_merge,reduce_then_merge

run_block merge_hpp_precision_mismatch_min_10_18 \
    --left-k-values 10 \
    --right-k-values 18 \
    --merge-strategies reject,unsafe_naive_merge,reduce_then_merge

run_block merge_hpp_precision_mismatch_min_18_10 \
    --left-k-values 18 \
    --right-k-values 10 \
    --merge-strategies reject,unsafe_naive_merge,reduce_then_merge

run_block merge_hpp_precision_mismatch_min_14_18 \
    --left-k-values 14 \
    --right-k-values 18 \
    --merge-strategies reject,unsafe_naive_merge,reduce_then_merge

run_block merge_hpp_precision_mismatch_min_18_14 \
    --left-k-values 18 \
    --right-k-values 14 \
    --merge-strategies reject,unsafe_naive_merge,reduce_then_merge

echo
echo "[done] minimal precision-mismatch matrix completed"
