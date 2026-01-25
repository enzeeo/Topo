"""
Functions for materializing the compute input parquet for C++.
"""

from pathlib import Path

import pandas as pd


def select_rolling_window_ending_on_or_before(
    df: pd.DataFrame,
    window_length: int,
    end_date: pd.Timestamp,
) -> pd.DataFrame:
    """
    Select the last `window_length` trading days ending on or before `end_date`.

    Args:
        df: Long-form DataFrame with columns [date, ticker, close].
        window_length: Number of trading days (price rows) to include.
        end_date: Inclusive end date (trading day) for the window.

    Returns:
        Filtered DataFrame covering the window dates only.
    """
    date_column = pd.to_datetime(df["date"])
    end_date_ts = pd.to_datetime(end_date)

    df_upto = df[date_column <= end_date_ts].copy()
    if df_upto.empty:
        return df_upto

    unique_dates = sorted(pd.to_datetime(df_upto["date"]).unique())
    if len(unique_dates) <= window_length:
        return df_upto

    cutoff_date = unique_dates[-window_length]
    return df_upto[pd.to_datetime(df_upto["date"]) >= cutoff_date]


def select_last_rolling_window_trading_days(
    df: pd.DataFrame,
    window_length: int,
) -> pd.DataFrame:
    """
    Select the most recent rolling_window_length trading days from the price data.

    Args:
        df: Long-form DataFrame with columns [date, ticker, close].
        window_length: Number of trading days to select.

    Returns:
        Filtered DataFrame containing only the last window_length dates.
    """
    date_column = df["date"]
    if date_column.dtype == "object":
        date_column = pd.to_datetime(date_column)
    
    unique_dates = sorted(date_column.unique())
    if len(unique_dates) <= window_length:
        return df
    
    cutoff_date = unique_dates[-window_length]
    filtered_dataframe = df[date_column >= cutoff_date]
    return filtered_dataframe


def pivot_long_to_wide_matrix(df: pd.DataFrame) -> pd.DataFrame:
    """
    Pivot long-form price data to wide matrix format.

    Args:
        df: Long-form DataFrame with columns [date, ticker, close].

    Returns:
        Wide DataFrame where:
        - Rows are dates (sorted ascending)
        - Columns are tickers (sorted alphabetically)
        - Values are close prices
    """
    wide_dataframe = df.pivot(index="date", columns="ticker", values="close")
    wide_dataframe = wide_dataframe.sort_index()
    wide_dataframe = wide_dataframe.sort_index(axis=1)
    return wide_dataframe


def write_prices_window_parquet(
    df: pd.DataFrame,
    output_path: Path,
) -> None:
    """
    Write the wide-format price window to parquet for C++ consumption.

    Args:
        df: Wide-format DataFrame [dates x tickers].
        output_path: Path to write the parquet file.
    """
    output_path.parent.mkdir(parents=True, exist_ok=True)
    df.to_parquet(output_path, index=True, engine="pyarrow")


def write_prices_window_parquets_for_dates(
    all_prices_long: pd.DataFrame,
    window_length: int,
    target_dates: list[pd.Timestamp],
    compute_inputs_dir: Path,
) -> list[Path]:
    """
    Materialize one compute-input parquet per trading date:
      compute_inputs/prices_window_YYYY-MM-DD.parquet

    Also writes/overwrites compute_inputs/prices_window.parquet as the latest window.

    Args:
        all_prices_long: Long-form DataFrame with [date, ticker, close] (entire warehouse).
        window_length: Number of trading days (price rows) in each window (e.g., 51).
        target_dates: Trading dates to materialize windows for.
        compute_inputs_dir: Directory to write compute inputs.

    Returns:
        List of paths written (dated windows, plus the latest alias).
    """
    if not target_dates:
        return []

    compute_inputs_dir.mkdir(parents=True, exist_ok=True)

    written: list[Path] = []
    sorted_dates = sorted(pd.to_datetime(target_dates))
    latest_date = sorted_dates[-1]

    for dt in sorted_dates:
        window_long = select_rolling_window_ending_on_or_before(all_prices_long, window_length, dt)
        unique_dates = sorted(pd.to_datetime(window_long["date"]).unique()) if not window_long.empty else []
        if len(unique_dates) != window_length:
            # Enforce strict window length so C++ always gets 51 price rows (50 returns).
            # If there isn't enough history yet for a given date, skip materializing it.
            continue

        wide = pivot_long_to_wide_matrix(window_long)
        if wide.shape[0] != window_length:
            continue

        date_string = pd.to_datetime(dt).date().isoformat()
        dated_path = compute_inputs_dir / f"prices_window_{date_string}.parquet"
        write_prices_window_parquet(wide, dated_path)
        written.append(dated_path)

    # Convenience alias for workflows / single-run compute (only if we wrote at least one dated file).
    if written:
        latest_written_dated = written[-1]
        latest_alias = compute_inputs_dir / "prices_window.parquet"
        latest_alias.write_bytes(latest_written_dated.read_bytes())
        written.append(latest_alias)

    return written
