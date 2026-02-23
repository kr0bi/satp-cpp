# Suggerimenti grafici scientifici (best-config)

## Configurazioni migliori selezionate dai CSV
- HyperLogLog: `k=16,L=32`
- HyperLogLog++: `k=18`
- LogLog: `k=15,L=32`
- Probabilistic Counting: `L=23`

## Grafici da includere
1. Traiettorie \(\hat{F}_0(t)\) vs \(F_0(t)\) in scala lineare e log-log (4 pannelli per i valori finali di \(F_0\)).
   - Asse x: `number_of_elements_processed`
   - Asse y: `f0_heat_mean_t` + `f0_mean_t`
   - Motivo: mostra accuratezza dinamica e fase transitoria.

2. Varianza nel tempo (log-log) sulle stesse best-config.
   - Asse x: `number_of_elements_processed`
   - Asse y: `variance`
   - Motivo: separa algoritmi con errore simile ma dispersione diversa.

3. Errore relativo medio finale per algoritmo e \(F_0\).
   - Asse x: valore finale di `f0`
   - Asse y: `mean_relative_error` al checkpoint finale
   - Motivo: confronto sintetico immediato per discussione in tesi.

4. Curva di sensibilità parametro->MRE finale per ciascun algoritmo.
   - Asse x: parametro (`k` o `L`)
   - Asse y: MRE medio endpoint
   - Motivo: giustifica la scelta best-config nel testo.

5. Confronto merge vs seriale (boxplot delta assoluto + conteggio non-zero).
   - Metriche: `delta_merge_serial_abs`, `delta_merge_serial_rel`
   - Motivo: verifica empirica della proprietà di mergeabilità implementata.

6. (Aggiuntivo consigliato) Scatter di calibrazione endpoint \(\hat{F}_0\) vs \(F_0\) con diagonale ideale.
   - Asse x: `f0` endpoint
   - Asse y: `f0_heat_mean_t` endpoint
   - Motivo: visualizza bias sistematico in modo diretto e leggibile.

7. (Aggiuntivo consigliato) Tempo di convergenza al 5\% di errore relativo.
   - Definizione: primo checkpoint t in cui `mean_relative_error <= 0.05`
   - Motivo: misura dinamica utile oltre all’endpoint.
