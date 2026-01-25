"""
Entry point for the daily ingestion pipeline.
"""

import json
import os
import sys
from datetime import date
from pathlib import Path

import pandas as pd

from .compute_input_materializer import (
    write_prices_window_parquets_for_dates,
)
from .config import ScraperConfig, ScraperResult
from .prices_store import (
    compute_missing_dates,
    ensure_directories,
    get_last_stored_date,
    write_prices_parquet,
)
from .qc_report import build_qc_report, write_qc_report_json
from .universe_loader import chunk_tickers, load_universe, validate_universe
from .validation import (
    coerce_price_types,
    count_bad_value_rows,
    dedupe_prices,
    find_missing_tickers_for_date,
)
from .yf_fetcher import fetch_prices_batched_yfinance, normalize_yfinance_to_long


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
    ensure_directories(config)
    
    universe = load_universe(config)
    validate_universe(universe)
    
    last_stored_date = get_last_stored_date(config.price_store_dir)
    missing_dates = compute_missing_dates(
        last_stored_date, run_date, config.lookback_buffer_days
    )
    
    if len(missing_dates) == 0:
        fetched_dates = []
        normalized_dataframe = None
    else:
        start_date = min(missing_dates)
        end_date = max(missing_dates)
        ticker_batches = chunk_tickers(universe.tickers, config.batch_size)
        raw_responses = fetch_prices_batched_yfinance(
            ticker_batches, start_date, end_date
        )
        normalized_dataframe = normalize_yfinance_to_long(
            raw_responses, config.prefer_adjusted
        )
        
        if normalized_dataframe.empty:
            print(f"Warning: No data fetched from yfinance for date range {start_date} to {end_date}")
            fetched_dates = []
        else:
            unique_date_series = normalized_dataframe["date"].unique()
            fetched_dates_list = []
            for single_date in unique_date_series:
                if isinstance(single_date, pd.Timestamp):
                    fetched_dates_list.append(single_date.date())
                elif hasattr(single_date, 'date'):
                    fetched_dates_list.append(single_date.date())
                else:
                    fetched_dates_list.append(single_date)
            fetched_dates = sorted(fetched_dates_list)
    
    if normalized_dataframe is not None and not normalized_dataframe.empty:
        cleaned_dataframe, duplicate_count = dedupe_prices(normalized_dataframe)
        bad_value_count = count_bad_value_rows(cleaned_dataframe)
        
        if bad_value_count > 0:
            raise ValueError(
                f"Found {bad_value_count} rows with invalid prices (NaN, inf, or <= 0)"
            )
        
        coerced_dataframe = coerce_price_types(cleaned_dataframe)
        dates_written = write_prices_parquet(coerced_dataframe, config.price_store_dir)
        
        if len(fetched_dates) > 0:
            most_recent_fetched_date = max(fetched_dates)
            
            # Merge with existing data to check for tickers that might have historical data
            prices_parquet_path = config.price_store_dir / "prices.parquet"
            if prices_parquet_path.exists():
                existing_dataframe = pd.read_parquet(prices_parquet_path)
                if not existing_dataframe.empty:
                    combined_for_check = pd.concat([existing_dataframe, coerced_dataframe], ignore_index=True)
                    combined_for_check = combined_for_check.drop_duplicates(subset=["date", "ticker"], keep="last")
                else:
                    combined_for_check = coerced_dataframe
            else:
                combined_for_check = coerced_dataframe
            
            missing_tickers = find_missing_tickers_for_date(
                combined_for_check, universe.tickers, most_recent_fetched_date
            )
            print(f"Fetched data for {len(coerced_dataframe)} rows across {len(fetched_dates)} dates")
            print(f"Checking missing tickers for most recent date: {most_recent_fetched_date}")
            print(f"Missing {len(missing_tickers)} tickers for {most_recent_fetched_date}")
            if len(missing_tickers) > 0 and len(missing_tickers) <= 10:
                print(f"Missing tickers: {missing_tickers}")
        else:
            missing_tickers = []
            print(f"Fetched data for {len(coerced_dataframe)} rows but no dates found")
    else:
        dates_written = []
        duplicate_count = 0
        bad_value_count = 0
        
        prices_parquet_path = config.price_store_dir / "prices.parquet"
        if prices_parquet_path.exists():
            existing_dataframe = pd.read_parquet(prices_parquet_path)
            if not existing_dataframe.empty:
                existing_dates = sorted(existing_dataframe["date"].unique())
                if len(existing_dates) > 0:
                    most_recent_existing_date = existing_dates[-1]
                    if isinstance(most_recent_existing_date, pd.Timestamp):
                        most_recent_existing_date = most_recent_existing_date.date()
                    missing_tickers = find_missing_tickers_for_date(
                        existing_dataframe, universe.tickers, most_recent_existing_date
                    )
                    print(f"No new data fetched. Checked existing data: {len(existing_dataframe)} rows, {len(missing_tickers)} tickers missing for {most_recent_existing_date}")
                else:
                    missing_tickers = universe.tickers
                    print(f"Warning: Existing parquet has no dates. All {len(universe.tickers)} tickers marked as missing.")
            else:
                missing_tickers = universe.tickers
                print(f"Warning: No data in existing parquet. All {len(universe.tickers)} tickers marked as missing.")
        else:
            missing_tickers = universe.tickers
            print(f"Warning: No data was fetched and no existing parquet found. All {len(universe.tickers)} tickers marked as missing.")
    
    # Update compute input only when new prices were written (skip on weekends / no new fetch).
    if len(dates_written) > 0:
        prices_parquet_path = config.price_store_dir / "prices.parquet"
        if prices_parquet_path.exists():
            all_prices_dataframe = pd.read_parquet(prices_parquet_path)
            # Materialize compute inputs only for trading dates (dates present in the warehouse).
            # Use 51 trading-day price rows so C++ can compute 50 return rows.
            compute_inputs_dir = config.price_store_dir.parent / "compute_inputs"
            target_dates = sorted(pd.to_datetime(coerced_dataframe["date"]).unique().tolist())
            if target_dates:
                written_paths = write_prices_window_parquets_for_dates(
                    all_prices_long=all_prices_dataframe,
                    window_length=51,
                    target_dates=target_dates,
                    compute_inputs_dir=compute_inputs_dir,
                )
                print(f"Materialized {len(written_paths)} compute input files under {compute_inputs_dir}")

    # Expose run outcome for GHA: C++ runs only when new_data is true.
    ingest_result_path = config.qc_out_dir.parent / "ingest_result.json"
    ingest_result = {
        "run_date": run_date.isoformat(),
        "new_data": len(dates_written) > 0,
    }
    ingest_result_path.parent.mkdir(parents=True, exist_ok=True)
    with open(ingest_result_path, "w") as f:
        json.dump(ingest_result, f, indent=2)

    notes_list = []
    if len(missing_tickers) > 0:
        notes_list.append(f"Missing data for {len(missing_tickers)} tickers on {run_date}")
    
    qc_report = build_qc_report(
        run_date=run_date,
        fetched_dates=fetched_dates,
        universe=universe,
        missing_tickers=missing_tickers,
        duplicate_rows=duplicate_count,
        bad_value_rows=bad_value_count,
        notes=notes_list,
    )
    
    # Only write QC report if new trading-day rows were fetched and written.
    if len(dates_written) > 0:
        write_qc_report_json(qc_report, config.qc_out_dir)
        print(f"QC report written for {run_date} (new data: {len(dates_written)} dates)")
    else:
        print(f"No new data fetched for {run_date}. Using existing data. Skipping QC report creation.")
    
    run_ok = len(missing_tickers) == 0 and bad_value_count == 0
    
    result = ScraperResult(
        ok=run_ok,
        run_date=run_date,
        dates_written=dates_written,
        qc=qc_report,
    )
    
    return result


if __name__ == "__main__":
    import pandas as pd
    from dateutil import tz
    
    if "RUN_DATE" in os.environ:
        run_date_string = os.environ["RUN_DATE"]
        run_date = date.fromisoformat(run_date_string)
    else:
        market_timezone = tz.gettz("America/New_York")
        today_pandas = pd.Timestamp.now(tz=market_timezone)
        run_date = today_pandas.date()
    
    project_root = Path(__file__).parent.parent
    universe_path = project_root / "data" / "universe.csv"
    price_store_dir = project_root / "data" / "prices"
    qc_out_dir = project_root / "out" / "qc"
    
    config = ScraperConfig(
        universe_path=universe_path,
        price_store_dir=price_store_dir,
        qc_out_dir=qc_out_dir,
        timezone="America/New_York",
        batch_size=50,
        lookback_buffer_days=5,
        prefer_adjusted=True,
    )
    
    result = run_daily_ingest(config, run_date)
    
    if result.ok:
        print(f"Ingestion successful for {result.run_date}")
        sys.exit(0)
    else:
        print(f"Ingestion completed with issues for {result.run_date}")
        print(f"Missing tickers: {len(result.qc.missing_tickers)}")
        sys.exit(1)
