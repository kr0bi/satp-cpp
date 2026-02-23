# Chapter 5 CSV Analysis (streaming + merge)

## Coverage
- streaming files: 72
- merge files: 72
- streaming rows: 288000
- merge rows: 36000
- sample_size values: [10000000]
- f0 values: [100000, 1000000, 5000000, 10000000]
- seeds: [42, 137357, 10032018, 21041998, 29042026]

## Final checkpoint summary (reference params)
| algorithm | params | mre_mean | rmse_mean | variance_mean |
|---|---|---:|---:|---:|
| HyperLogLog | k=16,L=32 | 0.002736079 | 10711.884799 | 73553991.258890 |
| HyperLogLog++ | k=16 | 0.002904721 | 10713.931838 | 73557119.219760 |
| LogLog | k=16,L=32 | 0.011063060 | 14962.368824 | 90443926.423235 |
| Probabilistic Counting | L=16 | 0.787552345 | 3944600.702673 | 98644414.550000 |

## Best parameter by mean final MRE
| algorithm | best_param_value | mre_mean |
|---|---:|---:|
| HyperLogLog | 16 | 0.002736079 |
| HyperLogLog++ | 18 | 0.001369746 |
| LogLog | 15 | 0.005614671 |
| Probabilistic Counting | 23 | 0.524950785 |

## Merge summary by algorithm
| algorithm | rows | mean_abs_delta | max_abs_delta | mean_rel_delta | max_rel_delta |
|---|---:|---:|---:|---:|---:|
| HyperLogLog | 6500 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| HyperLogLog++ | 7500 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| LogLog | 6500 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| Probabilistic Counting | 15500 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |

## HLL++ vs HLL (k=16) at final checkpoint
- abs_diff_hat: mean=15.714000, median=3.470000, max=71.740000
- abs_diff_mre: mean=0.000169400, median=0.000000778, max=0.000846200