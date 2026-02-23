# Evidence Log — Capitoli 4 e 5

## 1) File di codice consultati

### CLI / orchestrazione
- `/Users/daniele/CLionProjects/satp-cpp/main.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/cli/BenchmarkCli.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/cli/CliConfig.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/cli/CliTypes.h`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/cli/AlgorithmExecutor.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/cli/PathUtils.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/scripts/orchestrate_benchmarks.py`

### Dataset I/O
- `/Users/daniele/CLionProjects/satp-cpp/scripts/generate_partitioned_dataset_bin.py`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/io/BinaryDatasetIO.h`

### Framework simulazione / metriche / CSV
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/simulation/EvaluationFramework.h`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/simulation/EvaluationFramework.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/simulation/EvaluationFramework.tpp`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/simulation/CsvResultWriter.h`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/simulation/ErrorAccumulator.h`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/simulation/StreamingCheckpointBuilder.h`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/simulation/Stats.h`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/simulation/Loop.h`

### Algoritmi / hashing
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/algorithms/Algorithm.h`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/algorithms/HyperLogLogPlusPlus.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/algorithms/HyperLogLog.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/algorithms/LogLog.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/algorithms/ProbabilisticCounting.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/src/satp/hashing.h`

### Test / supporto
- `/Users/daniele/CLionProjects/satp-cpp/tests/simulation/EvaluationFrameworkTest.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/tests/algorithms/HyperLogLog.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/tests/algorithms/LogLogTest.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/tests/algorithms/ProbabilisticCountingTest.cpp`
- `/Users/daniele/CLionProjects/satp-cpp/tests/TestData.h`
- `/Users/daniele/CLionProjects/satp-cpp/README.md` (TODO e limiti)

## 2) Paper consultati

### Letti e utilizzabili
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/FlMa85.pdf`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/FlFuGaMe07.pdf`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/hll++.pdf`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/optimal_algorithm_distinct.pdf`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/mergeable_summaries.pdf`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/bar-yossef2002.pdf`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/Alon et al. - The space complexity of approximating the frequency moments.pdf`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/Data streams algorithms and applications.pdf`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/Cormode - 2017 - Data sketching.pdf`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/Prezza - 2025 - Algorithms for Massive Data -- Lecture Notes.pdf`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/MotwaniRandomized.pdf`

### Non pienamente parsabili / mancanti (TODO)
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/DuFl03.pdf` (estrazione testuale non affidabile)
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/bloom.pdf` (non trovato)
- `/Users/daniele/CLionProjects/satp-cpp/thesis/papers/cmencyc.pdf` (non trovato)

## 3) Mapping sezioni -> evidenze

## Capitolo 4 (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/4-implementazione.tex`)

- `\section{Architettura del sistema}` (linee ~18+)
  - Evidenza codice: `main.cpp`, `src/satp/cli/*.cpp`, `src/satp/simulation/*`, `src/satp/io/BinaryDatasetIO.h`, `scripts/orchestrate_benchmarks.py`.

- `\section{Dataset binario compresso}` e sottosezioni (linee ~57+)
  - Evidenza codice: `scripts/generate_partitioned_dataset_bin.py`, `src/satp/io/BinaryDatasetIO.h`.
  - Claim su bitset di verità prefissata `F0(t)`: `BinaryDatasetIO.h` + `EvaluationFramework.tpp`.

- `\section{Generazione dataset}` (linee ~129+)
  - Evidenza codice: `scripts/generate_partitioned_dataset_bin.py` (parametri CLI, naming, vincoli, workers).

- `\section{Interfacce comuni e hashing}` (linee ~153+)
  - Evidenza codice: `src/satp/algorithms/Algorithm.h`, `src/satp/hashing.h`.

- `\section{Implementazione degli algoritmi}` (linee ~185+)
  - Evidenza codice: `ProbabilisticCounting.cpp`, `LogLog.cpp`, `HyperLogLog.cpp`, `HyperLogLogPlusPlus.cpp`.
  - Evidenza teorica citata: `Flajolet_Martin_1985`, `Durand_Flajolet_2003`, `Flajolet_Fusy_Gandouet_Meunier_2007`, `Heule_Nunkesser_Hall_2013`.

- `\section{Framework di valutazione}` e `\subsection{Output CSV}` (linee ~254+)
  - Evidenza codice: `EvaluationFramework.*`, `ErrorAccumulator.h`, `StreamingCheckpointBuilder.h`, `CsvResultWriter.h`, `AlgorithmExecutor.cpp`.

- `\section{CLI e orchestrazione sperimentale}` (linee ~316+)
  - Evidenza codice: `BenchmarkCli.cpp`, `CliConfig.cpp`, `PathUtils.cpp`, `orchestrate_benchmarks.py`.

- `\section{Seed e riproducibilita'}` (linee ~352+)
  - Evidenza codice: `generate_partitioned_dataset_bin.py` -> `BinaryDatasetIO.h` -> `CliConfig.cpp` -> `EvaluationFramework.cpp` -> `CsvResultWriter.h`.

- `\section{Validazione e testing}` (linee ~375+)
  - Evidenza codice: `tests/algorithms/*.cpp`, `tests/simulation/EvaluationFrameworkTest.cpp`, `tests/TestData.h`.

- `\section{Scelte progettuali e limiti attuali}` (linee ~402+)
  - Evidenza codice + doc: `README.md` (TODO), coverage effettiva test.

## Capitolo 5 (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/5-risultati.tex`)

- `\section{Obiettivi e perimetro}` (linee ~12+)
  - Evidenza codice: run modes supportati in `BenchmarkCli.cpp`, `AlgorithmExecutor.cpp`.

- `\section{Piano sperimentale}` e sottosezioni (linee ~26+)
  - Evidenza codice: `orchestrate_benchmarks.py` (griglie n/d/seed/p, sweep parametri).

- `\section{Schema CSV atteso e controlli di consistenza}` (linee ~71+)
  - Evidenza codice: header reali in `CsvResultWriter.h`.
  - Evidenza operativa: output path da `PathUtils.cpp`.

- `\section{Metriche di stima, varianza ed errore}` (linee ~117+)
  - Evidenza teorica: definizioni da cap.2 (`2-background.tex`).
  - Evidenza implementativa: calcolo pratico in `ErrorAccumulator.h`.

- `\section{Analisi streaming (placeholder)}` (linee ~202+)
  - Evidenza codice: checkpoints in `StreamingCheckpointBuilder.h`, colonne streaming da `CsvResultWriter.h`.

- `\section{Analisi merge (placeholder)}` (linee ~274+)
  - Evidenza codice: merge stats in `EvaluationFramework.tpp` + `CsvResultWriter.h`.
  - Evidenza teorica: `mergeable_summaries.pdf`, `FlFuGaMe07.pdf`.

- `\section{Matrice di analisi e completamento dei placeholder}` (linee ~340+)
  - Evidenza metodo: piano in `/Users/daniele/CLionProjects/satp-cpp/notes/ch5_results_plan.md`.

## 4) Note su citazioni e bibliografia
- Le chiavi citate nei capitoli 4-5 sono presenti in `/Users/daniele/CLionProjects/satp-cpp/thesis/references.bib`.
- Non sono state inserite nuove entry bib in questo passaggio perché non necessarie ai capitoli riscritti.
- TODO bibliografia: se si vorranno usare `DuFl03`, `bloom.pdf`, `cmencyc.pdf`, servono fonti parsabili/metadati completi.
