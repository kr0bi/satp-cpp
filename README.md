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
- `src/satp/hashing/`: hash functions used by the sketches
- `src/satp/cli/Cli.h`: coordinator of the interactive benchmark CLI
- `src/satp/dataset/Dataset.h`: coordinator of binary dataset indexing and partition loading
- `src/satp/simulation/Simulation.h`: coordinator of the experiment framework
- `src/satp/*/detail/`: internal files grouped by sub-responsibility
- `main.cpp`: benchmark-style executable
- `tests/`: Catch2 unit tests

## Dataset format
The dataset file is binary and compressed per partition (`zlib`). The framework indexes partition metadata and loads/decompresses one partition at a time.

## Architectural flow
The main runtime flow is:

`Cli.h -> Dataset.h -> Simulation.h`

- `Cli` parses commands, loads dataset/runtime config, and orchestrates output paths and progress reporting.
- `Dataset` exposes partition-level reads and metadata from the binary dataset.
- `Simulation` runs streaming or merge experiments and emits in-memory statistics/CSV rows.

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
results/<namespace>/<mode>/<AlgorithmName>/<hash>/<params>/results_streaming.csv
results/<namespace>/<mode>/<AlgorithmName>/<hash>/<params>/results_merge.csv
```

Example:
```text
results/legacy/streaming/HyperLogLog++/splitmix64/k_16/results_streaming.csv
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
f0_mean_t,f0_hat_mean_t,variance,stddev,rse_theoretical,rse_observed,
bias,absolute_bias,relative_bias,mean_relative_error,rmse,mae
```

## Reproducibility
- Tests use the fixed dataset at `tests/data/dataset_n_2000_d_1000_p_3_s_5489.bin`.
- HLL/LogLog constructors validate parameter ranges (k/L) and have tests for invalid values.
- In binary mode, the evaluation framework reads `runs`, `sampleSize`, and `seed` directly from the dataset metadata.
- The streaming checkpoint planner now enforces an exact checkpoint budget and keeps coverage on both the early and late parts of the stream.

## Hash robustness analysis (scientific reproducibility)
Current hashing is deterministic (splitmix64), so results are reproducible for a fixed dataset. If you want to study robustness to different hash functions/seeds, introduce a configurable hash seed (e.g., `splitmix64(id ^ seed)`), log it in the CSV, and optionally centralize hashing in the framework.

## TODO
### Completato
- [x] Refactor del dataset su file unico per configurazione (`dataset_n_{n}_d_{d}_p_{p}_s_{seed}.bin`).
- [x] Formato binario compresso per partizione (`zlib`) con caricamento di una sola partizione alla volta.
- [x] Modalita' `streaming` e `merge` integrate nel framework.
- [x] Operazione di merge implementata e valutata in CSV dedicato (`results_merge.csv`).
- [x] Output CSV standardizzati e salvati automaticamente in `results/<namespace>/<mode>/<AlgorithmName>/<hash>/<params>/`.
- [x] Orchestrazione batch con sweep completo dei parametri (`scripts/orchestrate_benchmarks.py --full`).
- [x] Pipeline notebook per grafici e analisi sperimentale (streaming + merge).
- [x] Moduli `cli`, `dataset` e `simulation` riorganizzati con un coordinatore pubblico e helper in `detail/`.

### Da fare (priorita')
1. Fare refactoring e pulire il codice e la repository, in modo che sia chiaro cosa fa ogni componente.
2. Implementare il piano sperimentale sul merge eterogeneo con HyperLogLog++ come riferimento: baseline omogeneo, hash/seed mismatch, mismatch di precisione (`reject` vs `reduce-then-merge`) e amplificazione su topologie di merge. Roadmap operativa: `codex/merge_heterogeneous_roadmap.md`.
3. Estendere framework, CLI e CSV per esperimenti dedicati di merge eterogeneo (`valid`, `recoverable`, `invalid`) con confronto rispetto a baseline omogeneo e unione esatta.
4. Implementare Count-Min Sketch, Bloom Filter e Ring Bloom Filter.
5. Aggiungere ulteriori grafici per gli algoritmi attuali.

### Opzionali
1. Aggiungere test sulla dimensione degli sketch e analisi queste infomrazioni
2. Integrazione input da sistema streaming reale (es. Apache Kafka).
3. Campagne comparative su dataset non uniformi (oltre al caso uniforme corrente).
