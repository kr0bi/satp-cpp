#!/usr/bin/env bash
set -euo pipefail

# Generate prefix_constant_rho datasets with fixed n and varying d (rho = n / d).
# This script ONLY generates datasets and does not run benchmarks/framework.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GENERATOR="${ROOT_DIR}/scripts/generate_prefix_constant_rho_dataset_bin.py"
OUTPUT_DIR="${ROOT_DIR}/datasets/prefix_constant_rho"

N=10000000
P=50
SEED=21041998
WORKERS=8

# rho in {1,2,5,10,20,50,100} => d in {1e7,5e6,2e6,1e6,5e5,2e5,1e5}
D_VALUES=(10000000 5000000 2000000 1000000 500000 200000 100000)

if [[ ! -f "${GENERATOR}" ]]; then
  echo "[error] generator not found: ${GENERATOR}" >&2
  exit 1
fi

mkdir -p "${OUTPUT_DIR}"

TOTAL="${#D_VALUES[@]}"
GLOBAL_START="$(date +%s)"

echo "[start] generating ${TOTAL} datasets"
echo "        n=${N} p=${P} seed=${SEED} workers=${WORKERS}"
echo "        output=${OUTPUT_DIR}"

for i in "${!D_VALUES[@]}"; do
  IDX=$((i + 1))
  D="${D_VALUES[$i]}"
  RHO=$((N / D))
  PERCENT=$((IDX * 100 / TOTAL))

  ITER_START="$(date +%s)"
  echo ""
  echo "[${IDX}/${TOTAL}] ${PERCENT}%  d=${D}  rho=${RHO}"

  python3 "${GENERATOR}" \
    --output-dir "${OUTPUT_DIR}" \
    --n "${N}" \
    --d "${D}" \
    --p "${P}" \
    --seed "${SEED}" \
    --workers "${WORKERS}"

  ITER_END="$(date +%s)"
  ITER_SEC=$((ITER_END - ITER_START))
  ELAPSED=$((ITER_END - GLOBAL_START))
  echo "[done ${IDX}/${TOTAL}] d=${D} rho=${RHO}  iter=${ITER_SEC}s  elapsed=${ELAPSED}s"
done

GLOBAL_END="$(date +%s)"
TOTAL_SEC=$((GLOBAL_END - GLOBAL_START))
echo ""
echo "[completed] generated ${TOTAL}/${TOTAL} datasets in ${TOTAL_SEC}s"
