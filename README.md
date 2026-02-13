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

## Run tests
```sh
ctest --test-dir build
```

## Results CSV columns
The benchmark CSV includes standard error metrics plus observed/theoretical RSE:
```
algorithm,params,runs,sample_size,distinct_count,seed,
mean,variance,stddev,rse_theoretical,rse_observed,bias,difference,bias_relative,
mean_relative_error,rmse,mae
```

## Reproducibility
- Tests use the fixed dataset at `tests/data/dataset_n_2000_d_1000_p_3_s_5489.bin`.
- HLL/LogLog constructors validate parameter ranges (k/L) and have tests for invalid values.
- In binary mode, the evaluation framework reads `runs`, `sampleSize`, and `seed` directly from the dataset metadata.

## Hash robustness analysis (scientific reproducibility)
Current hashing is deterministic (splitmix64), so results are reproducible for a fixed dataset. If you want to study robustness to different hash functions/seeds, introduce a configurable hash seed (e.g., `splitmix64(id ^ seed)`), log it in the CSV, and optionally centralize hashing in the framework.

## TODO
1. Refactor della generazione dataset:
   - Dati `n` (numero totale di elementi), `d` (numero di elementi distinti), `p` (numero di partizioni) e `seed`.
   - Generare un unico file con nomenclatura `dataset_n_{n}_d_{d}_p_{p}_s_{seed}`.
   - Il file deve essere in formato JSON con lo schema seguente.
   - A differenza del metodo precedente, creare gia' tutte le partizioni dentro lo stesso file.
   - Obiettivo: eseguire tutti gli algoritmi su un singolo file pre-costruito.

   Schema JSON:
   ```json
   {
     "nOfElements": 5,
     "distinct": 4,
     "partizioni": [
       {
         "stream": [
           {
             "id": 1,
             "freq": 1
           },
           {
             "id": 2,
             "freq": 1
           },
           {
             "id": 100,
             "freq": 1
           },
           {
             "id": 4,
             "freq": 1
           },
           {
             "id": 1,
             "freq": 2
           }
         ]
       },
       {
         "stream": [
           {
             "id": 4,
             "freq": 1
           },
           {
             "id": 2,
             "freq": 1
           },
           {
             "id": 25,
             "freq": 1
           },
           {
             "id": 4,
             "freq": 2
           },
           {
             "id": 8,
             "freq": 1
           }
         ]
       }
     ]
   }
   ```

2. Aggiungere l'operazione di merge.
3. Estendere il framework con i seguenti workflow:
   - a) Dato un file dataset nel formato sopra, leggerlo e caricare una partizione alla volta in memoria; fissati `n`, `d` e `p`, misurare bias, varianza, media e altre metriche.
   - b) Abilitare una modalita' streaming: input da file intero, lettura elemento per elemento, calcolo progressivo delle metriche per osservare lo scostamento della stima nel tempo.
   - c) Abilitare una prima modalita' merge: merge di soli due sketch (inizialmente), confronto tra stima di `F_0` via merge e lettura seriale delle partizioni, sia con parametri fissi sia in modalita' streaming elemento per elemento.
4. Correggere la dicitura `Sketch Mergeable` in `Mergeable Sketch`.
5. Correggere la definizione dell'operatore di chiusura per il merge (attualmente errata).
6. Fare grafici teorici dello spazio consumato dagli sketch all'aumentare degli elementi.
7. Fare grafici variando numero di ID, parametri e merge, misurando precisione e varianza.
8. Implementare Count-Min Sketch e Bloom Filter.
9. Implementare Ring Bloom Filter.
10. Iniziare a produrre tutti i grafici sensati per l'analisi sperimentale.
11. Opzionali:
   - a) Fare grafici empirici dello spazio realmente usato da ogni sketch (thread/processo per algoritmo) al crescere degli elementi.
   - b) Fare merge di `k` cluster e misurare il deterioramento.
   - c) Usare un sistema concreto come Apache Kafka per gli input.
   - d) Aggiungere piu' funzioni di hash e rifare i grafici al variare della funzione.
   - e) Aggiungere serializzazione per abilitare scenari distribuiti.
