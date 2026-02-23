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
The dataset file is binary and compressed per partition (`zlib`). The framework indexes partition metadata and loads/decompresses one partition at a time.

## Generate datasets (Python)
```sh
python3 scripts/generate_partitioned_dataset_bin.py --output-dir . --n 1000000 --d 100000 --p 10 --seed 123
```
Then point the benchmark CLI to the dataset:
```
set datasetPath dataset_n_1000000_d_100000_p_10_s_123.bin
```

The generator writes values deterministically when a seed is provided and prints a progress bar on stderr by default.

## Build and run (CMake)
```sh
cmake -S . -B build
cmake --build build
./build/main
```

The CLI no longer takes a manual CSV output path. Results are written automatically under:
```text
results/<AlgorithmName>/<params>/results_oneshot.csv
results/<AlgorithmName>/<params>/results_streaming.csv
results/<AlgorithmName>/<params>/results_merge.csv
```

Example:
```text
results/HyperLogLog++/k_16/results_oneshot.csv
```

## Orchestration script
To generate a matrix of datasets and run benchmarks in batch:
```sh
python3 scripts/orchestrate_benchmarks.py
```

Useful options:
```sh
python3 scripts/orchestrate_benchmarks.py --skip-generate
python3 scripts/orchestrate_benchmarks.py --skip-streaming
python3 scripts/orchestrate_benchmarks.py --skip-merge
python3 scripts/orchestrate_benchmarks.py --clean-results
python3 scripts/orchestrate_benchmarks.py --k 16 --l 16 --l-log 32
python3 scripts/orchestrate_benchmarks.py --full
```

`--full` esegue automaticamente tutti i domini validi (paper-strict) per algoritmo:
- `HLL++`: `k` in `[4,18]`
- `HLL`: `k` in `[4,16]`, `L=32`
- `LogLog`: `k` in `[4,16]`, `L=32`
- `ProbabilisticCounting`: `L` in `[1,31]`

## Run tests
```sh
ctest --test-dir build
```

## Results CSV columns
The benchmark CSV includes standard error metrics plus observed/theoretical RSE:
```
algorithm,params,mode,runs,sample_size,number_of_elements_processed,f0,seed,
f0_mean_t,f0_heat_mean_t,variance,stddev,rse_theoretical,rse_observed,
bias,absolute_bias,relative_bias,mean_relative_error,rmse,mae
```

## Reproducibility
- Tests use the fixed dataset at `tests/data/dataset_n_2000_d_1000_p_3_s_5489.bin`.
- HLL/LogLog constructors validate parameter ranges (k/L) and have tests for invalid values.
- In binary mode, the evaluation framework reads `runs`, `sampleSize`, and `seed` directly from the dataset metadata.

## Hash robustness analysis (scientific reproducibility)
Current hashing is deterministic (splitmix64), so results are reproducible for a fixed dataset. If you want to study robustness to different hash functions/seeds, introduce a configurable hash seed (e.g., `splitmix64(id ^ seed)`), log it in the CSV, and optionally centralize hashing in the framework.

## TODO
### Completato
- [x] Refactor del dataset su file unico per configurazione (`dataset_n_{n}_d_{d}_p_{p}_s_{seed}.bin`).
- [x] Formato binario compresso per partizione (`zlib`) con caricamento di una sola partizione alla volta.
- [x] Modalita' `normal`, `streaming` e `merge` integrate nel framework.
- [x] Operazione di merge implementata e valutata in CSV dedicato (`results_merge.csv`).
- [x] Output CSV standardizzati e salvati automaticamente in `results/<AlgorithmName>/<params>/`.
- [x] Orchestrazione batch con sweep completo dei parametri (`scripts/orchestrate_benchmarks.py --full`).
- [x] Pipeline notebook per grafici e analisi sperimentale (streaming + merge).

### Da fare (priorita')
1. Implementare Count-Min Sketch (frequenze).
2. Implementare Bloom Filter (membership).
3. Implementare Ring Bloom Filter.
4. Estendere il merge da coppie a topologie con `k` nodi/cluster (catena/albero) e misurarne il degrado.
5. Aggiungere misure empiriche di costo:
   - tempo di `update`, `query`, `merge`;
   - memoria residente effettiva per algoritmo.
6. Introdurre robustezza all'hash sperimentale:
   - piu' funzioni di hash e/o seed di hash configurabile;
   - confronto dei grafici al variare dell'hash.
7. Aggiungere serializzazione/deserializzazione degli sketch per scenari distribuiti.

### Opzionali
1. Integrazione input da sistema streaming reale (es. Apache Kafka).
2. Campagne comparative su dataset non uniformi (oltre al caso uniforme corrente).
