"""
Functions for loading and validating the ticker universe.
"""

from typing import List, Sequence

from .config import ScraperConfig, Universe


def load_universe(config: ScraperConfig) -> Universe:
    """
    Load the ticker universe from disk.

    Args:
        config: Ingest configuration with universe_path.

    Returns:
        Universe containing tickers and a load timestamp.
    """
    raise NotImplementedError


def validate_universe(universe: Universe) -> None:
    """
    Validate that the universe is usable (non-empty, unique tickers, etc.).

    Args:
        universe: Universe object loaded from disk.

    Raises:
        ValueError: If universe is invalid.
    """
    raise NotImplementedError


def chunk_tickers(tickers: Sequence[str], batch_size: int) -> List[List[str]]:
    """
    Split a list of tickers into batches for yfinance requests.

    Args:
        tickers: Ticker symbols.
        batch_size: Max tickers per batch.

    Returns:
        List of ticker batches.
    """
    raise NotImplementedError
