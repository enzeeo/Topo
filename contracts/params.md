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
| `diffusion_eta` | 0.00536848 | Tune so historical avg smoothness ≈ 0.7 |

## Strain Index Coefficients

| Parameter | Value | Description |
|-----------|-------|-------------|
| `strain_coefficient_a` | 6.079022235067595 | Weight for L2 return magnitude |
| `strain_coefficient_b` | 7.9605896202281174 | Weight for systemic ratio |
| `strain_coefficient_c` | 1.1954996222800531 | Weight for Wasserstein distance |
| `strain_coefficient_d` | 0.1774031090998265 | Weight for total persistence |
| `strain_coefficient_e` | 0.0733645898241489 | Weight for graph total variation |

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

