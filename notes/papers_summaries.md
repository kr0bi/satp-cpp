# Paper summaries for chapters 4 (implementation choices) and 5 (experimental methodology)

## Flajolet & Martin (1985) — Probabilistic counting algorithms
- **Contributions:** Introduces PCSA, a compact streaming estimator for F0 that scans once, hashes each element, records the position of the least-significant 1-bit per hashed value, and builds a BITMAP whose leftmost zero encodes log2 n. Derives the bias correction φ ≈ 0.77351 and shows how to average multiple bitmaps for accuracy.
- **Relevant definitions/results:** BITMAP vector, COUNT/PCSA routine, E[R] ≈ log2 φ n, σ(R) ≈ 1.12; distributed merging by bitwise OR; scrolling/fringe encoding to transfer only the boundary of the mixed zone; extensions for deletions via counters.
- **Chapter 4 use:** Justifies maintaining m-bit bitmaps, hashing values uniformly, combining distributed substreams through bitwise OR, and compressing the fringe before transmission without losing accuracy. Demonstrates that PCSA needs only a few dozen assembly instructions per update and that low storage (≈k log2 n) gives relative error ≈10% (5% with 256 words), guiding register allocation and per-element cost budget in the implementation.
- **Chapter 5 use:** Empirical table of bias/standard error for different m (8...256) matches analytic predictions; experiments on 16 real files (Unix manual) show ratios of estimated vs exact counts; the paper quantifies bias/σ and shows that the estimator is insensitive to replication frequency, framing baseline expectations for synthetic vs real data.
Suggested bibkey: FlajoletMartin1985

## Flajolet et al. (2007) — HyperLogLog analysis
- **Contributions:** Defines HyperLogLog, which partitions hashed values into 2^b registers, tracks the maximum run of leading zeros per register, and combines them via a harmonic-mean-based indicator Z = (∑ 2^{-M[j]})^{-1} multiplied by a bias-correcting α_m to reach relative accuracy ≈1.03896/√m; proves this is near-optimal and outlines sliding-window/distributed adaptations.
- **Relevant definitions/results:** Ideal multiset model for hashing; stochastic averaging with a single hash; explicit algorithm (Figure 2) with register update, indicator Z, secondary corrections for small/large cardinals; section on parallelism/sliding window/real-world use cases.
- **Chapter 4 use:** Establishes the register-level computation (first b bits for bucket, remaining bits for ρ(w)), the use of a single hash function to emulate m experiments, the normalization constant α_m, and the ability to merge streams by taking component-wise maximums, which directly shapes the streaming implementation.
- **Chapter 5 use:** Presents simulations (histograms/Gaussian fits) for (n,m) pairs and real PostScript data (458 chunks of ≈40k lines) whose ±2% standard error matches theory; describes how error decreases as m grows and how sliding-window versions can be obtained, providing metrics for validation.
Suggested bibkey: Flajolet2007HyperLogLog

## Heule et al. (HyperLogLog++ paper)
- **Contributions:** Empirically motivated improvements to HyperLogLog: 64-bit hashing to avoid large-range correction, bias correction for small cardinalities via k-NN interpolation, dynamic switching between LinearCounting and corrected HLL, sparse representation (id, ρ) for n≪m, variable precision in sparse mode, and compressed encoding (variable-length + delta) to extend the sparse phase.
- **Relevant definitions/results:** Threshold selection (~11500 for p=14) for swapping between LC vs bias-corrected estimate; DecodeHash/EncodeHash for precision promotion/demotion; Table of max sparse pairs before falling back to dense; experimental measurements showing bias reduction (Figures 2–5) and error quantiles.
- **Chapter 4 use:** Provides concrete bookkeeping rules (64-bit hash, storing only max ρ per register, on-the-fly bias lookup, merging/threshold logic, sparse vs dense state, and efficient encoding) that dictate register layout, memory budgeting, and adaptive heuristics for the implementation.
- **Chapter 5 use:** Uses 5k random datasets per cardinality (precision 14) to measure bias, relative error, and quantiles; compares HLL, bias-corrected HLL, and LinearCounting, giving a methodology for choosing thresholds and justifying the use of interpolation-based bias correction on real workloads (Figure 5) with ~2% error.
Suggested bibkey: HeuleNunkesserHall2013

## Kane, Nelson & Woodruff (2010) — Optimal distinct elements algorithm
- **Contributions:** Presents the first algorithm achieving (1 ± ε) approximation for F0 with Θ(ε^{-2} + log n) bits and O(1) worst-case update/report time; extends to L0 (Hamming norm), providing constant-time, sublinear-space estimators even with deletions.
- **Relevant definitions/results:** Subsampling into geometrically increasing guesses R, maintaining bitmatrices per level, constant-time update/report by tracking bit-vectors for each guess, and proof that the space/time combination matches known lower bounds; generalization to L0 streams.
- **Chapter 4 use:** Motivates a bit-matrix hierarchy of guesses, storing only counters at the selected level and combining them, which justifies an architecture that multiplexes constant-time updates across log n levels using O(ε^{-2}) registers.
- **Chapter 5 use:** Theoretical baseline for accuracy: The order-ε variance guarantees (±εF0 with 2/3 probability) provide a rigorous target for experiments, and the argument about subsampling levels/bias informs how to measure and report baseline error when comparing heuristics.
Suggested bibkey: KaneNelsonWoodruff2010

## Agarwal et al. (Mergeable Summaries, PODS 2012)
- **Contributions:** Formalizes mergeability, shows MG/SpaceSaving deterministically mergeable while preserving O(1/ε) space and error, builds randomized mergeable quantile summaries of size O(ε^{-1} log^{3/2}(1/ε)), and extends to mergeable ε-approximations and ε-kernels (with pre-known directional width) for geometric data.
- **Relevant definitions/results:** Table 1 comparing summary sizes across offline/streaming/mergeable models; Lemma linking MG error to counter sums; merging algorithm that adds counters, subtracts k+1-th value, and prunes; mergeable quantile summary with restricted merging and generalizations to VC-dimension ν ranges.
- **Chapter 4 use:** Justifies implementing merge operations for MG/SpaceSaving summaries, designing fixed-size counters, and controlling error by tracking total counter weight (n̂) so merges do not accumulate extra ε; supports designing quantile/ε-approximation summaries that remain compact after merges.
- **Chapter 5 use:** Supplies theoretical guarantees (deterministic error ≤ n/(k+1), randomized quantile size-time trade-offs) that serve as baseline metrics and explain why merge count (operator associativity) can be treated as part of the error budget when framing distributed experiments.
Suggested bibkey: AgarwalCormodeHuangPhillipsWei2012

## Bar-Yossef et al. (2002) — Counting distinct elements in a data stream
- **Contributions:** Presents three algorithms with progressively better space/time trade-offs for (ε, δ)-approximating F0 in the streaming model; aims to reach poly(1/ε) + log m space, improving over earlier products of poly(1/ε)·log m.
- **Relevant definitions/results:** Definition of (ε, δ)-approximation over domain [m]; three algorithms giving spaces O(ε^{-1} log m), O(ε^{-2} + log m), and O(ε^{-1} + log m) (amortized) with corresponding update costs; lower-bounds discussion referencing communication complexity.
- **Chapter 4 use:** Highlights design patterns (sampling via random hash of elements, using minimum hashed value, using counters for subsampled bins) that inform the practical implementation of streaming estimators with provable additive error trade-offs.
- **Chapter 5 use:** Provides analytical bounds on failure probability and additive error for each variant, which can be used as reference baselines when calibrating experiments (e.g., verifying poly(ε^{-1}) scaling vs log m dependency).
Suggested bibkey: BarYossef2002

## Alon, Matias & Szegedy (1996/2002) — Frequency moments space complexity
- **Contributions:** Tight upper bounds for approximating Fk (space O(n^{1−1/k} log n)) and lower bounds showing Ω(n^{1−5/k}) for k ≥ 6, while F0/F1/F2 admit log-space algorithms; observes that Flajolet-Martin and Morris counters can be realized with simple hash families.
- **Relevant definitions/results:** Frequency moments Fk = ∑ m_i^k, definition of F*, use of 4-wise independent hash spaces, Chebyshev/Chernoff-based variance analysis; proofs that deterministic algorithms require Ω(n log m) to hold exact values.
- **Chapter 4 use:** Sets the target space budget for maintaining F0/F2 estimates and motivates the use of limited-independence hashing plus median-of-means-style repetition, explaining why implementations can rely on log-space structures for low-k moments but must avoid such techniques for k ≥ 6.
- **Chapter 5 use:** Theoretical accuracy/noise guarantees (Chebyshev) provide expected error regimes and signal that measuring higher-order moments is infeasible with small space, shaping what metrics to track experimentally.
Suggested bibkey: AlonMatiasSzegedy2002

## Muthukrishnan (Data Streams: Algorithms and Applications)
- **Contributions:** Comprehensive survey covering streaming models (turnstile, sliding window, windowed/stream permutations), key techniques (sampling, random projections, group testing, exponential histograms, robust approximations), system-level considerations, and new directions (functional approximation, hardware, privacy, data quality).
- **Relevant definitions/results:** Formal definitions of stream models, the Datar-Gionis-Indyk-Motwani exponential histogram for sliding windows (Section 5.2), summaries of lower bounds, and discussion of streaming systems/MapReduce analogues.
- **Chapter 4 use:** Provides the modeling language and architectural patterns (e.g., window maintenance, aggregation hierarchies, MapReduce-style reduce steps) that should guide implementation choices for revisiting cardinality and frequency sketches.
- **Chapter 5 use:** Offers experimental perspective via puzzles (missing numbers, fishing) and principles (e.g., sampling vs deterministic lower bounds), which help align experimental methodology—choice of window model, error metrics, and distributional assumptions.
Suggested bibkey: Muthukrishnan2005

## Cormode (2017) — Data sketching overview
- **Contributions:** Conceptual article framing sketching as the practical response to data overload; covers sampling, Bloom filters, Count-Min, HyperLogLog, merging strategies, and emerging applications such as social graph reachability; ties algorithmic innovations to production systems (advertising, networks).
- **Relevant definitions/results:** Trade-offs between sampling error 1/√s vs sketch error, Bloom filter false-positive analysis, HLL algorithm for log log n space, use of random tags for reservoir sampling, discussion of merging behavior, and anecdotal uses (Facebook separation defense, network monitoring).
- **Chapter 4 use:** Reinforces the engineering view that sketches must be mergeable, accumulate registers via max/or operations, and balance false positives (Bloom filter) vs accuracy (HyperLogLog), providing context for implementation-level choices (hash function quality, bitvector layout, constant α).
- **Chapter 5 use:** Documents how real deployments (network anomaly detection, advertising uniques, Facebook distance queries) calibrate error margins (e.g., 1–2% for HLL) and explains when sampling cannot answer cardinality questions, motivating metrics such as relative error and false-positive rate in experiments.
Suggested bibkey: Cormode2017

## Prezza (2025) — Algorithms for Massive Data (lecture notes)
- **Contributions:** Course notes covering compressed data structures, probabilistic tools/hashing, biased filters, LSH, pattern matching, frequency and cardinality estimation, relativistic dimension reduction, and stream-friendly computations (Chapter 5 includes Datar-Gionis-Indyk-Motwani’s algorithm, Count-Min, Misra-Gries, Flajolet-Martin, LogLog family, bottom-k, Morris counter, tug-of-war sketch, relational algebra sketches, and dimensionality reduction techniques).
- **Relevant definitions/results:** Compressed suffix arrays, Elias-Fano bitvectors, Bloom/quotient filters, similarity-preserving Rabin fingerprinting and MinHash, DGI-M sliding-window counters, frequency sketches, F0 algorithms, Fk/tug-of-war, and relational algebra sketches with explicit space/error bounds.
- **Chapter 4 use:** Offers concrete data structure blueprints (bitvector alignment, hashed counters, Bloom filters) and probability tools that support design decisions for implementing sketches, fingerprinting, and pattern matching in constrained memory.
- **Chapter 5 use:** Lays out streaming experiments (F0 accuracy, sliding-window queries, hashed counters, dimensionality reduction) with precise updating rules, letting the thesis adopt the described update/query sequences and error calculations as experimental methodology.
Suggested bibkey: Prezza2025

## Motwani & Raghavan (Randomized Algorithms, 1996 survey)
- **Contributions:** Surveys paradigms (foiling adversaries, random sampling, abundance of witnesses, fingerprinting/hashing, random reordering, load balancing, rapidly mixing Markov chains, isolation) that underpin randomized streaming/data summarization algorithms.
- **Relevant definitions/results:** Fingerprinting’s use in pattern matching, sampling’s error reduction, load balancing via randomized assignment, fingerprint-based membership tests, and isolation techniques for distributed symmetry breaking.
- **Chapter 4 use:** Justifies using randomness to defeat adversarial data orders, techniques like random reordering, hashing/fingerprinting of items, load balancing for distributed sketches, and rebalancing frequency counters when merging data.
- **Chapter 5 use:** Provides the methodological toolkit—sampling to reduce variance, fingerprinting for verifying equality, random reordering to avoid pathological inputs, and isolation for distributed merges—that determines how experiments should vary seeds and random inputs.
Suggested bibkey: MotwaniRaghavan1996

## Parsability issues
- **DuFl03.pdf:** Ghostscript/gs output contains mostly non-ASCII characters; unable to extract readable body—file marked TODO.
- **thesis/papers/bloom.pdf & thesis/papers/cmencyc.pdf:** Neither file exists under `thesis/papers`, so no parsing/summary was possible; if they should be included, please supply the missing PDFs or confirm their new path.

## Suggested citation placement map
- **Chapter 4 (implementation choices):** FlajoletMartin1985, Flajolet2007HyperLogLog, HeuleNunkesserHall2013, KaneNelsonWoodruff2010, AgarwalCormodeHuangPhillipsWei2012, BarYossef2002, AlonMatiasSzegedy2002, Prezza2025, Cormode2017, MotwaniRaghavan1996, Muthukrishnan2005.
- **Chapter 5 (experimental methodology & metrics):** FlajoletMartin1985, Flajolet2007HyperLogLog, HeuleNunkesserHall2013, AgarwalCormodeHuangPhillipsWei2012, BarYossef2002, Prezza2025, Cormode2017, Muthukrishnan2005, MotwaniRaghavan1996, KaneNelsonWoodruff2010.

## Non evidenziato (claims lacking current paper support)
- Seed-sensitivity experiments (variance of an estimator over multiple independent hash seeds) are not provided in these sources; the papers report theoretical variance only, so any claim about empirical seed robustness must be verified separately.
- Detailed merge-error accumulation beyond MG/SpaceSaving (e.g., after many distributed merges or different sketch families) is not measured except for PCSA and HLL summaries; claims about merge-error stability for HLL++ or other sketches thus require new experiments.
