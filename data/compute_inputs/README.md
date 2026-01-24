# Compute Inputs (Backfill)

Place one parquet file per trading date:

- `prices_window_YYYY-MM-DD.parquet`

Each file should contain the rolling window of prices required by `cpp/build/daily_run`.

