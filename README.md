# satp-cpp

C++ project that implements and evaluates probabilistic algorithms for estimating the number of distinct elements in a stream. It includes multiple counting algorithms, a small evaluation framework for benchmarking, and a Catch2-based test suite.

## Features
- Exact counting (NaiveCounting)
- Probabilistic Counting
- LogLog
- HyperLogLog
- HyperLogLog++
- Evaluation framework for repeated sampling and statistics (CSV output with error metrics)

## Project layout
- `src/satp/algorithms/`: implementations of the counting algorithms
- `src/satp/simulation/`: evaluation and loop utilities
- `src/satp/io/`: dataset I/O helpers
- `main.cpp`: benchmark-style executable
- `tests/`: Catch2 unit tests

## Dataset format
The dataset file is plain text:
```
<total_elements> <distinct_elements>
<value_1>
<value_2>
...
```

## Generate datasets (Python)
```sh
python3 scripts/generate_dataset.py --output dataset.txt --total 10000 --unique 1000 --seed 123
```
Then point the benchmark CLI to the dataset:
```
set datasetPath dataset.txt
```

The generator writes values deterministically when a seed is provided and prints a progress bar on stderr by default.

## Build and run (CMake)
```sh
cmake -S . -B build
cmake --build build
./build/main
```

## Run tests
```sh
ctest --test-dir build
```

## Results CSV columns
The benchmark CSV includes standard error metrics plus observed/theoretical RSE:
```
algorithm,params,runs,sample_size,dataset_size,distinct_count,seed,
mean,variance,stddev,rse_theoretical,rse_observed,bias,difference,bias_relative,
mean_relative_error,rmse,mae
```

## Reproducibility
- Tests use the fixed dataset at `tests/data/dataset_10k_1k.txt`.
- HLL/LogLog constructors validate parameter ranges (k/L) and have tests for invalid values.
- The evaluation framework caches generated subsets and regenerates them if `runs` or `sampleSize` changes between evaluations.

## Hash robustness analysis (scientific reproducibility)
Current hashing is deterministic (splitmix64), so results are reproducible for a fixed dataset. If you want to study robustness to different hash functions/seeds, introduce a configurable hash seed (e.g., `splitmix64(id ^ seed)`), log it in the CSV, and optionally centralize hashing in the framework.

## Work to do
- Hash robustness analysis: add a configurable hash seed (e.g., `splitmix64(id ^ seed)`), log it in the CSV, and optionally centralize hashing in the framework.
- Distributed/merge core (thesis):
  - Interface for mergeable sketches: `merge(const Sketch&)`, `serialize/deserialize`, `size_bytes()`, `params()`, `clone()`.
  - Implement merge:
    - HLL/HLL++/LogLog: register-wise max.
    - ProbabilisticCounting: bitmap OR.
    - NaiveCounting: union set (exact baseline).
  - Property tests:
    - Commutativity, associativity, idempotence (where applicable).
    - Equivalence: insert-all vs merge-of-partitions.
    - Stability across topology (chain vs tree).
- Error-vs-cardinality and per-run logging:
  - Export per-run metrics (estimate, truth, rel_error, run_id, seed, params, dataset_id).
  - Sweep across cardinalities (log-scale x) instead of a single fixed dataset.
  - Confidence intervals / bootstrapping for error bands (offline or in-script).
- Optional (discuss with advisor): realistic partitioning for merge experiments:
  - Generate P shards with controlled cross-shard duplicates (not uniform sampling).
  - Run merge experiments across different topologies (chain vs tree).
- Optional (discuss with advisor): performance/footprint metrics:
  - Separate timing for update / estimate / merge.
  - RAM footprint and serialized payload size.
  - (Optional) allocation counts / memory counters.
