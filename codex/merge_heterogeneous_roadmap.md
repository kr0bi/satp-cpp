# SATP-C++ Heterogeneous Merge Roadmap

Generated on: **2026-03-07 CET**

Questo documento traduce il piano di tesi sul merge eterogeneo in una roadmap operativa del repository.

## 1) Scope

Focus primario:

- famiglia HLL-like con **HyperLogLog++ come riferimento principale**
- studio della compatibilita' del merge sotto eterogeneita' di hash e precisione
- classificazione dei casi in:
  - `valid`
  - `recoverable`
  - `invalid`

Fuori scope esplicito:

- implementare UltraLogLog
- implementare ExaLogLog
- allargare subito il confronto a tutte le famiglie di sketch
- usare `dense vs sparse` come asse principale della tesi

## 2) Research framing operativo

La domanda da implementare nel codice e negli esperimenti e':

> Quando il merge di sketch di cardinalita' HLL-like e' semanticamente valido sotto eterogeneita' di hash e precisione, quanto errore introduce quando non lo e', e quali trasformazioni sono sufficienti per recuperare i casi sanabili?

Assi sperimentali da mantenere:

1. `hash mismatch`
2. `precision mismatch`
3. `topology amplification`

Asse opzionale, solo dopo i primi tre:

4. `non-ideal hashing / structured inputs`

## 3) Stato attuale del repository

Vincoli gia' presenti nel codice:

- il merge corrente e' pairwise su partizioni `(0,1), (2,3), ...`
- il framework corrente assume un singolo contesto omogeneo per hash/parametri
- `HyperLogLog`, `LogLog` e `HyperLogLog++` rigettano gia' i mismatch di parametri nel merge
- il runtime supporta gia' hash multipli: `splitmix64`, `xxhash64`, `murmurhash3`, `siphash24`

File chiave da considerare:

- `src/satp/simulation/detail/merge/MergeEvaluation.tpp`
- `src/satp/simulation/detail/results/CsvResultWriter.h`
- `src/satp/cli/detail/Cli.cpp`
- `src/satp/cli/detail/config/RunParameters.cpp`
- `src/satp/cli/detail/execution/AlgorithmRunner.h`
- `src/satp/hashing/HashFactory.cpp`
- `src/satp/algorithms/HyperLogLogPlusPlus.cpp`
- `tests/simulation/EvaluationFrameworkTest.cpp`
- `tests/algorithms/HyperLogLog.cpp`

## 4) Success criteria

La roadmap e' completata quando il repository consente di:

1. confermare il baseline omogeneo del merge dopo il refactor
2. eseguire benchmark di merge eterogeneo con HLL++
3. distinguere chiaramente `valid`, `recoverable`, `invalid`
4. esportare CSV dedicati con metadati sufficienti per i grafici della tesi
5. misurare almeno una topologia multi-step oltre al merge pairwise

## 5) Phases

### Phase 0 - Homogeneous control

Obiettivo:

- ristabilire il controllo sperimentale prima dell'eterogeneita'

Tasks:

- rieseguire `runmerge` omogeneo per HLL++ su una piccola griglia di `p`
- rieseguire il baseline anche sulle hash gia' supportate
- verificare che `delta_merge_serial_abs` e `delta_merge_serial_rel` restino circa nulli

Output:

- tabella/baseline omogeneo usata come controllo in tutti gli esperimenti successivi

### Phase 1 - Data model e CSV dedicati

Obiettivo:

- introdurre una rappresentazione esplicita del merge eterogeneo senza sovraccaricare lo schema corrente

Tasks:

- aggiungere tipi dedicati sotto `src/satp/simulation/detail/merge/`
- estendere `CsvResultWriter` con uno schema specifico per il merge eterogeneo
- includere almeno questi campi:
  - `algorithm`
  - `left_hash`, `right_hash`
  - `left_seed`, `right_seed`
  - `left_params`, `right_params`
  - `strategy`
  - `validity`
  - `estimate_merge`
  - `exact_union`
  - `error_rel_exact`
  - `baseline_homogeneous`
  - `delta_vs_baseline`
  - `topology`

Nota:

- non riusare direttamente il vecchio `results_merge.csv`

### Phase 2 - API/framework per merge eterogeneo pairwise

Obiettivo:

- introdurre nel framework una modalita' pairwise che possa istanziare sketch locali con contesti diversi

Tasks:

- aggiungere un evaluator dedicato accanto a `MergeEvaluation.tpp`
- consentire due configurazioni locali distinte per i due sketch
- mantenere separata la logica di:
  - creazione sketch
  - strategia di merge
  - baseline omogeneo
  - confronto con unione esatta

Deliverable minimo:

- valutazione pairwise eterogenea su HLL++

### Phase 3 - Surface CLI e script batch

Obiettivo:

- rendere gli esperimenti eseguibili senza manipolare a mano il framework

Tasks:

- aggiungere una surface minima nel CLI per il merge eterogeneo
- preferire un comando dedicato, ad esempio `runmergehet`, invece di complicare `runmerge`
- aggiungere uno script batch dedicato, ad esempio:
  - `scripts/orchestrate_heterogeneous_merge.py`

Questo script deve poter sweepare:

- hash family
- seed
- `p`
- strategia di merge
- topologia

### Phase 4 - Hash mismatch

Obiettivo:

- studiare la rottura semantica del merge register-wise sotto hash eterogenee

Ordine consigliato:

1. stesso hash, stessa seed (controllo)
2. stesso hash, seed diverse
3. hash diverse, stessa seed quando applicabile
4. hash diverse, seed diverse

Metriche minime:

- errore relativo rispetto all'unione esatta
- delta rispetto al baseline omogeneo
- classificazione `valid/recoverable/invalid`

Dataset:

- partire da dataset uniformi gia' disponibili

### Phase 5 - Precision mismatch su HLL++

Obiettivo:

- studiare quando il mismatch di `p` e' da rigettare e quando e' eventualmente recuperabile

Strategie da confrontare:

1. `reject`
2. `unsafe_naive_merge` (solo se isolato come modalita' sperimentale esplicita)
3. `reduce_then_merge`

Baseline corretto:

- confronto rispetto al baseline omogeneo alla precisione piu' bassa

Vincolo di implementazione:

- evitare refactor invasivi nel core algoritmico
- se serve una riduzione di precisione, mantenerla come helper esplicito e ben testato su HLL++

### Phase 6 - Topology amplification

Obiettivo:

- capire come l'errore si accumula lungo piu' livelli di merge

Topologie minime:

1. `pairwise` (gia' base)
2. `balanced_tree`
3. `left_deep_chain`

Misure:

- errore relativo finale
- delta rispetto al baseline omogeneo
- andamento dell'errore in funzione della profondita'

Nota:

- non introdurre topologie aggiuntive finche' pairwise eterogeneo non e' stabile

### Phase 7 - Optional stress: non-ideal hashing / structured inputs

Obiettivo:

- verificare il comportamento in presenza di input che violano o stressano l'ipotesi di hashing uniforme

Da fare solo dopo:

- hash mismatch
- precision mismatch
- topology amplification

Possibili workload:

- chiavi sequenziali con prefissi correlati
- skew/Zipf
- dataset sintetici con pattern strutturati

## 6) Testing strategy

Test minimi da aggiungere:

1. unit test su classificazione `valid/recoverable/invalid`
2. unit test su CSV header e righe del merge eterogeneo
3. test pairwise con due hash distinte e ground truth esatto
4. test su `reject` per mismatch di `p`
5. test sul percorso `reduce_then_merge` se implementato
6. test su almeno una topologia multi-step

File candidati:

- `tests/simulation/EvaluationFrameworkTest.cpp`
- nuovo file dedicato, ad esempio `tests/simulation/HeterogeneousMergeTest.cpp`

## 7) Analysis and notebooks

Dopo l'implementazione minima:

- aggiungere notebook o script di post-processing separati dai notebook streaming
- non mescolare i grafici del merge eterogeneo con i notebook Type A/Type B

Output attesi:

- matrice di compatibilita'
- curve di errore per hash mismatch
- curve di errore per precision mismatch
- grafici errore-vs-profondita' per le topologie

## 8) Recommended implementation order

Ordine operativo consigliato:

1. `Phase 1` - tipi e CSV
2. `Phase 2` - evaluator pairwise eterogeneo
3. `Phase 4` - hash mismatch
4. `Phase 5` - precision mismatch
5. `Phase 3` - CLI/script batch stabile
6. `Phase 6` - topologie multi-step
7. `Phase 7` - stress opzionale

Motivazione:

- `hash mismatch` sfrutta subito il supporto hash gia' presente
- `precision mismatch` richiede una semantica sperimentale piu' delicata
- le topologie hanno senso solo dopo che il pairwise e' definito bene

## 9) Non-goals for now

Per evitare dispersione, NON fare subito:

- implementazione di UltraLogLog o ExaLogLog
- confronto completo con tutte le altre famiglie di sketch
- campagne grandi su molte topologie contemporaneamente
- integrazione con workload reali prima di chiudere la pipeline sintetica
