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
