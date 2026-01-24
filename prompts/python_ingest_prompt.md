# Python Ingest Prompt

## Scope (Python ONLY)

You handle data ingestion and preparation.

You do NOT:

- Compute correlations
- Compute topology
- Implement math from the paper
- Modify C++ code

---

## Readability Requirements

- Every variable name must describe its purpose in English.
- Avoid abbreviations.
- Avoid list/dict comprehensions unless trivial.
- Use step-by-step logic.
- Intermediate variables are REQUIRED.

---

## Responsibilities

- Load ticker universe from `data/universe.csv`
- Fetch missing close prices only
- Validate:
  - All expected tickers present
  - No duplicate rows
  - No invalid or non-positive prices
- Write partitioned parquet store:
  - `data/prices/prices.parquet`
- Materialize compute input parquet:
  - `data/compute_inputs/prices_window.parquet`

---

## Materialization Rules

- Must contain exactly `rolling_window_length` rows
- Must contain exactly `number_of_assets` columns
- Must be dense (no missing values)
- Rows sorted by date
- Columns sorted by ticker

---

## Outputs

- Parquet warehouse
- Compute-input parquet
- QC report JSON

---

## Error Handling

- If validation fails → raise explicit exception
- No silent fixes
- No implicit forward-fill or interpolation

---

## Goal

Produce clean, explicit, readable data for C++.
Nothing else.
