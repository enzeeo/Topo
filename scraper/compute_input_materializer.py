"""
Functions for materializing the compute input parquet for C++.
"""

from pathlib import Path

import pandas as pd


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
