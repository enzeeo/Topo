"""
Functions for loading and validating the ticker universe.
"""

from datetime import datetime
from typing import List, Sequence

import pandas as pd

from .config import ScraperConfig, Universe


def load_universe(config: ScraperConfig) -> Universe:
    """
    Load the ticker universe from disk.

    Args:
        config: Ingest configuration with universe_path.

    Returns:
        Universe containing tickers and a load timestamp.
    """
    universe_dataframe = pd.read_csv(config.universe_path)
    ticker_column_name = "ticker"
    ticker_series = universe_dataframe[ticker_column_name]
    ticker_list = ticker_series.tolist()
    load_timestamp = datetime.now()
    universe = Universe(tickers=ticker_list, as_of=load_timestamp)
    return universe


def validate_universe(universe: Universe) -> None:
    """
    Validate that the universe is usable (non-empty, unique tickers, etc.).

    Args:
        universe: Universe object loaded from disk.

    Raises:
        ValueError: If universe is invalid.
    """
    if len(universe.tickers) == 0:
        raise ValueError("Universe is empty: no tickers found")
    
    unique_tickers = set(universe.tickers)
    if len(unique_tickers) != len(universe.tickers):
        raise ValueError("Universe contains duplicate tickers")
    
    for ticker in universe.tickers:
        if not isinstance(ticker, str):
            raise ValueError(f"Invalid ticker type: {ticker} is not a string")
        if len(ticker.strip()) == 0:
            raise ValueError("Universe contains empty ticker string")


def chunk_tickers(tickers: Sequence[str], batch_size: int) -> List[List[str]]:
    """
    Split a list of tickers into batches for yfinance requests.

    Args:
        tickers: Ticker symbols.
        batch_size: Max tickers per batch.

    Returns:
        List of ticker batches.
    """
    ticker_list = list(tickers)
    batches = []
    current_index = 0
    
    while current_index < len(ticker_list):
        end_index = current_index + batch_size
        batch = ticker_list[current_index:end_index]
        batches.append(batch)
        current_index = end_index
    
    return batches
