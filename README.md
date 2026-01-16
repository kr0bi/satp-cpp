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
- `src/satp/io/`: dataset naming and binary I/O
- `main.cpp`: benchmark-style executable
- `tests/`: Catch2 unit tests

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

## Docker (Ubuntu)
Build the image:
```sh
docker build -t satp-cpp .
```

Run the main executable:
```sh
docker run --rm satp-cpp main
```

Run the test suite:
```sh
docker run --rm satp-cpp test
```

## Notes
- The benchmark in `main.cpp` caches generated datasets in the repository root (e.g. `dataset_*.bin`).
- If your machine runs out of memory when building dependencies, try a single-threaded build: `cmake --build build -j1`.
