"""
Functions for validating and cleaning price data.
"""

from datetime import date
from typing import List, Sequence, Tuple

import numpy as np
import pandas as pd


def dedupe_prices(df: pd.DataFrame) -> Tuple[pd.DataFrame, int]:
    """
    Remove duplicate (date, ticker) rows.

    Args:
        df: Long-form DataFrame.

    Returns:
        Tuple of (deduped_df, duplicate_count).
    """
    initial_row_count = len(df)
    deduped_dataframe = df.drop_duplicates(subset=["date", "ticker"], keep="first")
    final_row_count = len(deduped_dataframe)
    duplicate_count = initial_row_count - final_row_count
    return deduped_dataframe, duplicate_count


def find_missing_tickers_for_date(
    df: pd.DataFrame,
    tickers: Sequence[str],
    target_date: date,
) -> List[str]:
    """
    Identify which tickers are missing data for a given target date.

    Args:
        df: Long-form DataFrame of fetched data.
        tickers: Full universe tickers.
        target_date: Date to check.

    Returns:
        List of tickers missing that date.
    """
    target_date_pandas = pd.to_datetime(target_date)
    date_filtered_dataframe = df[df["date"] == target_date_pandas]
    tickers_with_data = set(date_filtered_dataframe["ticker"].unique())
    all_tickers_set = set(tickers)
    missing_tickers = sorted(list(all_tickers_set - tickers_with_data))
    return missing_tickers


def count_bad_value_rows(df: pd.DataFrame) -> int:
    """
    Count rows with invalid numeric values (NaN, inf, <= 0) in key price columns.

    Args:
        df: Long-form DataFrame.

    Returns:
        Count of invalid rows.
    """
    close_column = df["close"]
    nan_mask = close_column.isna()
    infinite_mask = np.isinf(close_column)
    non_positive_mask = close_column <= 0
    bad_value_mask = nan_mask | infinite_mask | non_positive_mask
    bad_value_count = bad_value_mask.sum()
    return int(bad_value_count)


def coerce_price_types(df: pd.DataFrame) -> pd.DataFrame:
    """
    Ensure stable dtypes for storage and downstream compute.

    Args:
        df: Long-form DataFrame.

    Returns:
        DataFrame with coerced dtypes.
    """
    coerced_dataframe = df.copy()
    coerced_dataframe["date"] = pd.to_datetime(coerced_dataframe["date"]).dt.date
    coerced_dataframe["ticker"] = coerced_dataframe["ticker"].astype(str)
    coerced_dataframe["close"] = coerced_dataframe["close"].astype(float)
    return coerced_dataframe
