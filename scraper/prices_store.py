"""
Functions for reading and writing the Parquet price store.
"""

from datetime import date
from pathlib import Path
from typing import List, Optional

import pandas as pd

from .config import ScraperConfig


def ensure_directories(config: ScraperConfig) -> None:
    """
    Ensure required directories exist: price store, QC output, temp directories.

    Args:
        config: Ingest configuration.
    """
    raise NotImplementedError


def get_last_stored_date(price_store_dir: Path) -> Optional[date]:
    """
    Inspect the Parquet store and return the most recent stored trading date.

    Args:
        price_store_dir: Root directory containing Parquet price data.

    Returns:
        Most recent date found, or None if store is empty.
    """
    raise NotImplementedError


def compute_missing_dates(
    last_stored: Optional[date],
    today: date,
    lookback_buffer_days: int,
) -> List[date]:
    """
    Compute which dates should be fetched given the last stored date and today's date.

    Args:
        last_stored: Most recent stored date, or None.
        today: Current date in market timezone context.
        lookback_buffer_days: Extra days to include to handle late postings/corrections.

    Returns:
        List of dates to fetch (may be empty).
    """
    raise NotImplementedError


def write_prices_parquet(
    df: pd.DataFrame,
    price_store_dir: Path,
) -> List[date]:
    """
    Append new price rows to the Parquet store.

    Args:
        df: Long-form DataFrame with columns [date, ticker, close].
        price_store_dir: Root parquet directory.

    Returns:
        List of dates successfully written.
    """
    raise NotImplementedError
