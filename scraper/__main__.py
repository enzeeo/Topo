"""
Entry point for the daily ingestion pipeline.
"""

from datetime import date

from .config import ScraperConfig, ScraperResult


def run_daily_ingest(config: ScraperConfig, run_date: date) -> ScraperResult:
    """
    Run the full daily ingestion pipeline.

    Steps:
        1. Ensure directories exist
        2. Load and validate universe
        3. Compute missing dates to fetch
        4. Fetch from yfinance (batched)
        5. Normalize and validate
        6. Write to parquet store
        7. Materialize compute input for C++
        8. Write QC report

    Args:
        config: Ingest configuration.
        run_date: Logical market date for the run (usually "today" in market TZ).

    Returns:
        ScraperResult containing status, dates written, and QC report.
    """
    raise NotImplementedError


if __name__ == "__main__":
    # Entry point when running: python -m scraper
    raise NotImplementedError
