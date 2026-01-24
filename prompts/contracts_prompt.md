# Contracts Prompt

## Purpose

This document defines immutable contracts between Python and C++.

## Readability Rules Apply

- Variable names in examples must be fully descriptive.
- No shortened symbols unless mathematically standard (e.g., i, j in matrices).

---

## Canonical Parameters

- `number_of_assets` = 500
- `rolling_window_length` = 50
- `trading_days_only` = true
- `date_order` = ascending

---

## Parquet Compute Input Contract

- **File path**: `data/compute_inputs/prices_window.parquet`
- **Shape**:
  - Rows = `rolling_window_length`
  - Columns = `number_of_assets`
- **Row meaning**:
  - Each row = one trading day
  - Sorted ascending by date
- **Column meaning**:
  - Each column = one ticker
  - Sorted deterministically (alphabetically)
- **Values**:
  - Close prices
  - Strictly positive
  - No NaN or infinite values

---

## Binary Artifact Contracts

### returns.bin

- uint32 `number_of_assets`
- uint32 `rolling_window_length`
- double[rolling_window_length * number_of_assets] row-major

### corr.bin

- uint32 `number_of_assets`
- double[number_of_assets * number_of_assets] row-major

### diagram.bin

- uint32 `number_of_pairs`
- sequence of (double `birth_time`, double `death_time`)

---

## Final Output Contract

### strain.json fields:

- `date`
- `l2_return_magnitude`
- `graph_total_variation`
- `systemic_ratio`
- `total_persistence`
- `wasserstein_distance`
- `strain_index`

---

## Mathematical Invariants

- **Log return**: `log(current_price / previous_price)`
- **Correlation to distance**: `sqrt(2 * (1 - correlation_value))`
- **Persistent homology**: Dimension H1 only
- **Strain index**: Linear combination using fixed coefficients

---

## Prohibited Changes

- No renaming of fields
- No alternative metrics
- No implicit normalization
- No parameter changes

STOP if clarification is required.
