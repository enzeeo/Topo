"""
Scraper module for Market Strain Index data ingestion.

This module handles:
- Loading ticker universe
- Fetching missing prices from yfinance
- Validating and cleaning data
- Writing to Parquet storage
- Materializing compute inputs for C++
"""
