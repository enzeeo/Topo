"""
Functions for fetching price data from yfinance.
"""

from datetime import date
from typing import Any, List, Sequence

import pandas as pd


def fetch_prices_yfinance(
    tickers: Sequence[str],
    start: date,
    end: date,
) -> Any:
    """
    Fetch price data using yfinance for a given ticker list and date range.

    Args:
        tickers: Tickers to fetch.
        start: Inclusive start date.
        end: Exclusive end date.

    Returns:
        Raw yfinance response object (kept for debugging/traceability).
    """
    raise NotImplementedError


def fetch_prices_batched_yfinance(
    ticker_batches: Sequence[Sequence[str]],
    start: date,
    end: date,
) -> List[Any]:
    """
    Fetch price data in batches to reduce rate-limit risk.

    Args:
        ticker_batches: List of batches of tickers.
        start: Inclusive start date.
        end: Exclusive end date.

    Returns:
        List of raw yfinance response objects, one per batch.
    """
    raise NotImplementedError


def normalize_yfinance_to_long(
    raw_responses: Sequence[Any],
    prefer_adjusted: bool,
) -> pd.DataFrame:
    """
    Normalize raw yfinance outputs to a long-form DataFrame with a stable schema.

    Target schema columns:
        - date (datetime64[ns] or date)
        - ticker (string)
        - close (float64)

    Args:
        raw_responses: Raw yfinance outputs (batched).
        prefer_adjusted: Whether to prefer adjusted close.

    Returns:
        Long-form DataFrame with stable column names and types.
    """
    raise NotImplementedError
