# Chapter 5 Results Plan

## Scope
- Il capitolo 5 resta in modalità scaffold fino a quando i CSV non sono completi.
- Nessun risultato numerico o grafico finale deve essere inserito prima della validazione dati.
- I placeholder nel capitolo sono identificati con i label LaTeX indicati nella sezione TODO.

## Expected CSV paths
- Root risultati: `results/<AlgorithmName>/<params>/`.
- File normal: `results/<AlgorithmName>/<params>/results_oneshot.csv`.
- File streaming: `results/<AlgorithmName>/<params>/results_streaming.csv`.
- File merge: `results/<AlgorithmName>/<params>/results_merge.csv`.
- Dataset bin attesi (orchestrazione): `datasets/uniform_distribution/shuffled/random/dataset_n_<n>_d_<d>_p_<p>_s_<seed>.bin`.
- Algoritmi attesi in `<AlgorithmName>`: `HyperLogLog++`, `HyperLogLog`, `LogLog`, `ProbabilisticCounting`.
- Param folder attesi (sanitized):
- `k_<k>` per HLL++.
- `k_<k>_L_32` per HLL e LogLog.
- `L_<L>` per ProbabilisticCounting.

## Expected columns

### Normal and streaming CSV
```text
algorithm,params,mode,runs,sample_size,number_of_elements_processed,f0,seed,
f0_mean_t,f0_heat_mean_t,variance,stddev,rse_theoretical,rse_observed,
bias,absolute_bias,relative_bias,mean_relative_error,rmse,mae
```

### Merge CSV
```text
algorithm,params,mode,pairs,sample_size,pair_index,seed,
estimate_merge,estimate_serial,delta_merge_serial_abs,delta_merge_serial_rel
```

### Consistency checks (must pass before analysis)
- Header esatto e nello stesso ordine previsto.
- Nessun nullo nelle chiavi: `algorithm`, `params`, `mode`, `seed`.
- `mode` coerente con il file (`normal`/`streaming`/`merge`).
- Metriche non negative dove applicabile: `variance`, `mean_relative_error`, `rmse`, `mae`, `delta_merge_serial_abs`, `delta_merge_serial_rel`.
- Checkpoint streaming monotoni in `number_of_elements_processed`.
- Copertura endpoint streaming presente (`number_of_elements_processed == sample_size`).

## Scripts and commands generating CSV

### Build
```bash
cmake -S . -B build
cmake --build build
```

### Dataset generation (single case)
```bash
python3 scripts/generate_partitioned_dataset_bin.py \
  --output-dir datasets/uniform_distribution/shuffled/random \
  --n 10000000 --d 1000000 --p 50 --seed 21041998
```

### Batch orchestration (recommended)
```bash
python3 scripts/orchestrate_benchmarks.py
```

### Batch orchestration variants
```bash
python3 scripts/orchestrate_benchmarks.py --skip-generate
python3 scripts/orchestrate_benchmarks.py --clean-results
python3 scripts/orchestrate_benchmarks.py --full
python3 scripts/orchestrate_benchmarks.py --skip-streaming
python3 scripts/orchestrate_benchmarks.py --skip-merge
```

### Manual CLI run (single dataset)
```bash
./build/main <<'CLI'
set datasetPath datasets/uniform_distribution/shuffled/random/dataset_n_10000000_d_1000000_p_50_s_21041998.bin
set k 16
set l 16
set lLog 32
run all
runstream all
runmerge all
quit
CLI
```

## Aggregation plan

### Grouping keys
- Base keys: `algorithm`, `params`, `mode`, `seed`.
- Dataset keys: `sample_size` (n per partizione), `f0` (distinti finali), `runs` (partizioni).
- Streaming key: `number_of_elements_processed`.
- Merge key: `pair_index`.

### Metrics from chapter 2 to compute/report
- `bias`, `absolute_bias`, `relative_bias`.
- `variance`, `stddev`, `rse_observed`, confronto con `rse_theoretical`.
- `mean_relative_error`, `rmse`, `mae`.
- Merge-specific: `delta_merge_serial_abs`, `delta_merge_serial_rel`.

### Aggregation levels
- Livello A (intra-seed): confronto algoritmi/parametri a seed fissato.
- Livello B (inter-seed): aggregazione su seed per robustezza statistica.
- Livello C (cross-cardinality): confronto su diversi `f0` o rapporti `d/n`.

## Analysis matrix

| Research question | Mode | Grouping | Output placeholder |
|---|---|---|---|
| Accuratezza endpoint per algoritmo | normal | `algorithm,params,seed,f0` | `tab:res-endpoint-metrics-placeholder` |
| Coerenza RSE teorica vs osservata | normal/streaming | `algorithm,params,seed,f0` | `tab:res-rse-theo-obs-placeholder` |
| Dinamica del transitorio | streaming | `algorithm,params,seed,number_of_elements_processed` | `fig:res-streaming-stima-linear-placeholder`, `fig:res-streaming-stima-log-placeholder`, `fig:res-streaming-var-linear-placeholder`, `fig:res-streaming-var-log-placeholder`, `tab:res-streaming-transient-placeholder` |
| Stabilità del merge | merge | `algorithm,params,seed,pair_index` | `tab:res-merge-summary-placeholder`, `fig:res-merge-delta-placeholder` |
| Sanity e integrità dei CSV | all | `mode` | `tab:res-csv-quality-checks` |

## TODO tied to chapter placeholders
- [ ] `tab:res-csv-quality-checks`: compilare esiti PASS/FAIL dopo validazione header/chiavi/domini metriche.
- [ ] `tab:res-endpoint-metrics-placeholder`: inserire metriche endpoint aggregate per algoritmo e parametri.
- [ ] `tab:res-rse-theo-obs-placeholder`: inserire confronto RSE teorica/osservata.
- [ ] `fig:res-streaming-stima-linear-placeholder`: inserire grafico stima vs verità in scala lineare.
- [ ] `fig:res-streaming-stima-log-placeholder`: inserire grafico stima vs verità in scala log-log.
- [ ] `fig:res-streaming-var-linear-placeholder`: inserire grafico varianza in scala lineare.
- [ ] `fig:res-streaming-var-log-placeholder`: inserire grafico varianza in scala log-log.
- [ ] `tab:res-streaming-transient-placeholder`: inserire sintesi numerica del transitorio (checkpoint iniziali e globali).
- [ ] `tab:res-merge-summary-placeholder`: inserire sintesi dei delta merge vs seriale.
- [ ] `fig:res-merge-delta-placeholder`: inserire distribuzione degli scarti merge/seriale.
- [ ] `tab:res-analysis-matrix`: aggiornare stato completamento per ciascuna domanda sperimentale.
