"""
Functions for reading and writing the Parquet price store.
"""

from datetime import date, timedelta
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
    config.price_store_dir.mkdir(parents=True, exist_ok=True)
    config.qc_out_dir.mkdir(parents=True, exist_ok=True)
    compute_inputs_dir = config.price_store_dir.parent / "compute_inputs"
    compute_inputs_dir.mkdir(parents=True, exist_ok=True)


def get_last_stored_date(price_store_dir: Path) -> Optional[date]:
    """
    Inspect the Parquet store and return the most recent stored trading date.

    Args:
        price_store_dir: Root directory containing Parquet price data.

    Returns:
        Most recent date found, or None if store is empty.
    """
    parquet_file_path = price_store_dir / "prices.parquet"
    
    if not parquet_file_path.exists():
        return None
    
    stored_dataframe = pd.read_parquet(parquet_file_path)
    
    if stored_dataframe.empty:
        return None
    
    date_column = stored_dataframe["date"]
    if isinstance(date_column.iloc[0], pd.Timestamp):
        date_column = pd.to_datetime(date_column).dt.date
    max_date = date_column.max()
    
    if isinstance(max_date, pd.Timestamp):
        return max_date.date()
    return max_date


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
    if last_stored is None:
        start_date = today - timedelta(days=lookback_buffer_days + 30)
    else:
        start_date = last_stored + timedelta(days=1)
    
    end_date = today + timedelta(days=1)
    
    if start_date >= end_date:
        return []
    
    date_range = pd.date_range(start=start_date, end=end_date, freq="D")
    date_list = [single_date.date() for single_date in date_range]
    return date_list


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
    parquet_file_path = price_store_dir / "prices.parquet"
    
    # If no new rows, do not claim any dates were written.
    if df.empty:
        return []

    # Dates represented by the incoming dataframe (these are the "written" dates).
    incoming_dates = sorted(pd.to_datetime(df["date"]).dt.date.unique().tolist())

    if parquet_file_path.exists():
        existing_dataframe = pd.read_parquet(parquet_file_path)
        combined_dataframe = pd.concat([existing_dataframe, df], ignore_index=True)
        combined_dataframe = combined_dataframe.drop_duplicates(subset=["date", "ticker"], keep="last")
    else:
        combined_dataframe = df.copy()

    combined_dataframe = combined_dataframe.sort_values(by=["date", "ticker"])
    combined_dataframe.to_parquet(parquet_file_path, index=False, engine="pyarrow")

    return incoming_dates
