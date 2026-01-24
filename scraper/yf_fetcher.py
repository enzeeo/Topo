"""
Functions for fetching price data from yfinance.
"""

from datetime import date
from typing import List, Sequence

import pandas as pd
import yfinance as yf


def fetch_prices_yfinance(
    tickers: Sequence[str],
    start: date,
    end: date,
) -> pd.DataFrame:
    """
    Fetch price data using yfinance for a given ticker list and date range.

    Args:
        tickers: Tickers to fetch.
        start: Inclusive start date.
        end: Exclusive end date.

    Returns:
        Raw yfinance DataFrame (kept for debugging/traceability).
    """
    ticker_list = list(tickers)
    start_datetime = pd.Timestamp(start)
    end_datetime = pd.Timestamp(end) + pd.Timedelta(days=1)
    
    if len(ticker_list) == 1:
        ticker_object = yf.Ticker(ticker_list[0])
        historical_data = ticker_object.history(start=start_datetime, end=end_datetime)
        historical_data.columns = pd.MultiIndex.from_product([[ticker_list[0]], historical_data.columns])
    else:
        historical_data = yf.download(
            ticker_list,
            start=start_datetime,
            end=end_datetime,
            progress=False,
            group_by="ticker"
        )
    
    return historical_data


def fetch_prices_batched_yfinance(
    ticker_batches: Sequence[Sequence[str]],
    start: date,
    end: date,
) -> List[pd.DataFrame]:
    """
    Fetch price data in batches to reduce rate-limit risk.

    Args:
        ticker_batches: List of batches of tickers.
        start: Inclusive start date.
        end: Exclusive end date.

    Returns:
        List of raw yfinance DataFrames, one per batch.
    """
    raw_responses = []
    for batch in ticker_batches:
        batch_response = fetch_prices_yfinance(batch, start, end)
        raw_responses.append(batch_response)
    return raw_responses


def normalize_yfinance_to_long(
    raw_responses: Sequence[pd.DataFrame],
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
    all_dataframes = []
    
    for historical_data in raw_responses:
        if historical_data.empty:
            continue
        
        if isinstance(historical_data.columns, pd.MultiIndex):
            ticker_symbols = historical_data.columns.get_level_values(0).unique()
            for ticker_symbol in ticker_symbols:
                ticker_data = historical_data[ticker_symbol]
                
                if prefer_adjusted and "Adj Close" in ticker_data.columns:
                    price_column = ticker_data["Adj Close"]
                elif "Close" in ticker_data.columns:
                    price_column = ticker_data["Close"]
                else:
                    continue
                
                date_index = ticker_data.index
                date_values = [single_date.date() for single_date in date_index]
                
                ticker_dataframe = pd.DataFrame({
                    "date": date_values,
                    "ticker": ticker_symbol,
                    "close": price_column.values
                })
                all_dataframes.append(ticker_dataframe)
        else:
            if prefer_adjusted and "Adj Close" in historical_data.columns:
                price_column = historical_data["Adj Close"]
            elif "Close" in historical_data.columns:
                price_column = historical_data["Close"]
            else:
                continue
            
            date_index = historical_data.index
            date_values = [single_date.date() for single_date in date_index]
            
            ticker_symbol = "UNKNOWN"
            if len(historical_data.columns) > 0:
                first_column = historical_data.columns[0]
                if isinstance(first_column, tuple):
                    ticker_symbol = first_column[0]
                elif isinstance(first_column, str):
                    ticker_symbol = first_column
            
            ticker_dataframe = pd.DataFrame({
                "date": date_values,
                "ticker": ticker_symbol,
                "close": price_column.values
            })
            all_dataframes.append(ticker_dataframe)
    
    if len(all_dataframes) == 0:
        return pd.DataFrame(columns=["date", "ticker", "close"])
    
    combined_dataframe = pd.concat(all_dataframes, ignore_index=True)
    combined_dataframe["date"] = pd.to_datetime(combined_dataframe["date"])
    combined_dataframe["ticker"] = combined_dataframe["ticker"].astype(str)
    combined_dataframe["close"] = combined_dataframe["close"].astype(float)
    
    return combined_dataframe
