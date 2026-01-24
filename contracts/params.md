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
| `diffusion_eta` | TBD | Tune so historical avg smoothness ≈ 0.7 |

## Strain Index Coefficients

| Parameter | Value | Description |
|-----------|-------|-------------|
| `strain_coefficient_a` | 0.84 | Weight for L2 return magnitude |
| `strain_coefficient_b` | 0.53 | Weight for systemic ratio |
| `strain_coefficient_c` | 0.31 | Weight for Wasserstein distance |
| `strain_coefficient_d` | 0.19 | Weight for total persistence |

## Invariants

- All outputs must be deterministic given the same input
- Stable ordering for tickers (alphabetical) and dates (ascending)
- No randomness without fixed seed
- Identical results across runs
