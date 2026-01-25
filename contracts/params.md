# Canonical Parameters

## Core Constants

| Parameter | Value | Description |
|-----------|-------|-------------|
| `number_of_assets` | 500 | Number of stocks (S&P 500) |
| `rolling_window_length` | 50 | Days in rolling window |
| `trading_days_only` | true | Exclude weekends/holidays |
| `date_order` | ascending | Row ordering in matrices |

## Diffusion Parameter

| Parameter | Value | Description |
|-----------|-------|-------------|
| `diffusion_eta` | 0.00245267 | Tuned on 2022–2023 (train window) |

## Strain Index Coefficients

| Parameter | Value | Description |
|-----------|-------|-------------|
| `strain_coefficient_a` | 0.004369387935587976 | Weight for L2 return magnitude (tuned on 2022–2023) |
| `strain_coefficient_b` | 0.050109667888410196 | Weight for systemic ratio (tuned on 2022–2023) |
| `strain_coefficient_c` | -0.001537519524304718 | Weight for Wasserstein distance (tuned on 2022–2023) |
| `strain_coefficient_d` | -0.00019748477849821104 | Weight for total persistence (tuned on 2022–2023) |
| `strain_coefficient_e` | -0.00005095294446977887 | Weight for graph total variation (tuned on 2022–2023) |

## Historical Strain Index Stats

Computed from `out/daily/date=*/strain.json` `strain_index` values (same scale).

| Parameter | Value | Description |
|-----------|-------|-------------|
| `strain_index_mean` | 14.988931691254296 | Mean of daily `strain_index` |
| `strain_index_std_pop` | 3.1382172910274146 | Population standard deviation of daily `strain_index` |

## Invariants

- All outputs must be deterministic given the same input
- Stable ordering for tickers (alphabetical) and dates (ascending)
- No randomness without fixed seed
- Identical results across runs

