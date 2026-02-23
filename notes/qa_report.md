# ConsistencyQA Report

## Scope
- Reviewed files:
  - `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/4-implementazione.tex`
  - `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/5-risultati.tex`
- Reference inputs:
  - `/Users/daniele/CLionProjects/satp-cpp/notes/style_guide_extracted.md`
  - `/Users/daniele/CLionProjects/satp-cpp/notes/code_architecture_map.md`
  - `/Users/daniele/CLionProjects/satp-cpp/notes/papers_summaries.md`
  - `/Users/daniele/CLionProjects/satp-cpp/notes/ch5_results_plan.md`

## Findings (ordered by severity)
1. **High** — Over-claim in chapter 4 test coverage and merge behavior wording.
   - The text implied full domain-validation coverage for all algorithms and strict merge equivalence semantics, while code/tests show: explicit bound tests for HLL/HLL++/LogLog, and tolerance-based checks for HLL++ merge behavior.
   - Impact: documentation-code mismatch.

2. **Medium** — Missing nearby citations in chapter 4 for theory-backed claims.
   - Claims involving PC constant behavior, LogLog/HLL family regimes, HLL++ correction logic, and theoretical RSE constants were present without adjacent citations.
   - Impact: citation coherence weaker than chapters 2-3 conventions.

3. **Medium** — Chapter 5 placeholders were present but some TODO fields were generic.
   - Several checklist cells used `TODO: PASS/FAIL + nota` without explicit expected evidence fields.
   - Impact: reduced actionability for final fill-in.

4. **Low** — Numerical/graph result leakage check.
   - No measured numerical results and no generated-graph claims were found in chapter 5.
   - Impact: none; requirement satisfied.

## Fixes applied
- Updated `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/4-implementazione.tex`:
  - softened/clarified test-coverage and merge-behavior wording to match actual tests;
  - added citations next to theoretical claims (`Flajolet_Martin_1985`, `Durand_Flajolet_2003`, `Flajolet_Fusy_Gandouet_Meunier_2007`, `Heule_Nunkesser_Hall_2013`);
  - clarified theoretical RSE assignment as implemented in `AlgorithmExecutor`;
  - added explicit limitation note about missing constructor-bound test for ProbabilisticCounting.

- Updated `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/5-risultati.tex`:
  - made CSV QA checklist TODO cells actionable (explicit evidence expected);
  - clarified `Probabilistic Counting` theoretical RSE placeholder as `NaN (atteso dal writer CSV)`;
  - added completion criteria block for each placeholder (source CSV, aggregation rule, final output label).

## Residual risks / TODO
- A dedicated unit test for ProbabilisticCounting constructor bounds (`L in [1,31]`) is still missing in codebase tests.
- Chapter 5 remains scaffold-only by design; final tables/figures still depend on validated CSV availability.
- No full LaTeX build was executed in this QA pass; syntax/format integration should be verified in the next compile run.
