"""
Functions for validating and cleaning price data.
"""

from datetime import date
from typing import List, Sequence, Tuple

import pandas as pd


def dedupe_prices(df: pd.DataFrame) -> Tuple[pd.DataFrame, int]:
    """
    Remove duplicate (date, ticker) rows.

    Args:
        df: Long-form DataFrame.

    Returns:
        Tuple of (deduped_df, duplicate_count).
    """
    raise NotImplementedError


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
    raise NotImplementedError


def count_bad_value_rows(df: pd.DataFrame) -> int:
    """
    Count rows with invalid numeric values (NaN, inf, <= 0) in key price columns.

    Args:
        df: Long-form DataFrame.

    Returns:
        Count of invalid rows.
    """
    raise NotImplementedError


def coerce_price_types(df: pd.DataFrame) -> pd.DataFrame:
    """
    Ensure stable dtypes for storage and downstream compute.

    Args:
        df: Long-form DataFrame.

    Returns:
        DataFrame with coerced dtypes.
    """
    raise NotImplementedError
