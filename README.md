# satp-cpp

C++ project that implements and evaluates probabilistic algorithms for estimating the number of distinct elements in a stream. It includes multiple counting algorithms, a small evaluation framework for benchmarking, and a Catch2-based test suite.

## Features
- Exact counting (NaiveCounting)
- Probabilistic Counting
- LogLog
- HyperLogLog
- HyperLogLog++
- Evaluation framework for repeated sampling and statistics

## Project layout
- `src/satp/algorithms/`: implementations of the counting algorithms
- `src/satp/simulation/`: evaluation and loop utilities
- `src/satp/io/`: dataset naming and dataset I/O helpers
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
