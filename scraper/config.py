"""
Configuration dataclasses for the scraper module.
"""

from dataclasses import dataclass
from datetime import date, datetime
from pathlib import Path
from typing import List


@dataclass(frozen=True)
class Universe:
    """
    Universe of tickers to ingest.

    Attributes:
        tickers: List of ticker symbols.
        as_of: Timestamp of when the universe was loaded.
    """
    tickers: List[str]
    as_of: datetime


@dataclass(frozen=True)
class ScraperConfig:
    """
    Configuration for daily price ingestion.

    Attributes:
        universe_path: CSV containing the ticker universe.
        price_store_dir: Root directory for Parquet price storage.
        qc_out_dir: Directory for QC reports and run artifacts.
        timezone: Market timezone label (used for consistent 'date' interpretation).
        batch_size: Number of tickers per yfinance request batch.
        lookback_buffer_days: Extra days to include when fetching to handle delays/corrections.
        prefer_adjusted: If True, use adjusted close for core series.
    """
    universe_path: Path
    price_store_dir: Path
    qc_out_dir: Path
    timezone: str
    batch_size: int
    lookback_buffer_days: int
    prefer_adjusted: bool


@dataclass(frozen=True)
class QCReport:
    """
    Quality control report for an ingestion run.

    Attributes:
        run_date: The logical run date (market date) the pipeline targeted.
        fetched_dates: Dates that were fetched from the remote source.
        n_tickers: Total number of tickers in the universe.
        missing_tickers: Tickers missing data for the target date.
        duplicate_rows: Count of duplicate (date, ticker) rows detected after normalization.
        bad_value_rows: Count of rows with invalid numeric values (NaN/inf/<=0).
        notes: Human-readable notes or warnings.
    """
    run_date: date
    fetched_dates: List[date]
    n_tickers: int
    missing_tickers: List[str]
    duplicate_rows: int
    bad_value_rows: int
    notes: List[str]


@dataclass(frozen=True)
class ScraperResult:
    """
    Final result of a daily ingestion run.

    Attributes:
        ok: Whether the run completed successfully.
        run_date: Logical market date targeted.
        dates_written: Dates successfully written to storage.
        qc: Quality control report for this run.
    """
    ok: bool
    run_date: date
    dates_written: List[date]
    qc: QCReport
