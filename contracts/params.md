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
| `strain_coefficient_a` | TBD | Weight for L2 return magnitude |
| `strain_coefficient_b` | TBD | Weight for systemic ratio |
| `strain_coefficient_c` | TBD | Weight for Wasserstein distance |
| `strain_coefficient_d` | TBD | Weight for total persistence |

## Invariants

- All outputs must be deterministic given the same input
- Stable ordering for tickers (alphabetical) and dates (ascending)
- No randomness without fixed seed
- Identical results across runs
