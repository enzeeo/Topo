# Parquet Schema Contracts

## Price Warehouse: `data/prices/prices.parquet`

**Format**: Long table (Python-owned warehouse)

| Column | Type | Description |
|--------|------|-------------|
| `date` | date | Trading date |
| `ticker` | string | Stock ticker symbol |
| `close` | float64 | Closing price |

**Rules**:
- Must be append-safe and dedupe-safe
- No duplicate (date, ticker) pairs
- All prices must be strictly positive
- No NaN or infinite values

---

## Compute Input: `data/compute_inputs/prices_window.parquet`

**Format**: Wide matrix (exact input for C++)

| Property | Value |
|----------|-------|
| Shape | `[rolling_window_length x number_of_assets]` |
| Rows | Trading dates, sorted ascending |
| Columns | Ticker symbols, sorted alphabetically |
| Values | Close prices (float64) |

**Rules**:
- Must contain exactly `rolling_window_length` rows (50)
- Must contain exactly `number_of_assets` columns (500)
- Must be dense (no missing values)
- All values strictly positive
- No NaN or infinite values

**Derivation**:
1. Select the most recent `rolling_window_length` trading days from prices.parquet
2. Pivot from long to wide format
3. Sort rows by date ascending
4. Sort columns by ticker alphabetically
