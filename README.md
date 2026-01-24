# Topo - Market Strain Index

A Market Strain Index (MSI) calculator using algebraic topology to measure structural resilience of financial markets.

## Overview

This project calculates a quantitative indicator that measures structural resilience of financial markets by analyzing the correlation network among stocks, using persistent homology from algebraic topology.

**Key Features:**
- Structural perspective on risk (vs. price-variability focus of VIX)
- Early-warning signal capabilities
- Complements existing volatility indices

## Architecture

```
Python (scraper/)          →    Parquet    →    C++ (cpp/)
- Data ingestion                               - Log returns
- yfinance fetching                            - Correlation matrix
- Validation                                   - Graph Laplacian
- Parquet generation                           - Persistent homology
                                               - Strain index
```

## Pipeline Flow

1. **Python Ingest** (daily, GitHub Actions)
   - Fetch missing prices from yfinance
   - Validate and clean data
   - Write to `data/prices/prices.parquet`
   - Materialize `data/compute_inputs/prices_window.parquet`

2. **C++ Compute** (triggered after ingest)
   - Read price window parquet
   - Compute log returns → correlation → graph → topology
   - Output `out/daily/date=YYYY-MM-DD/strain.json`

## Folder Structure

```
Topo/
├── contracts/          # Immutable contracts (schemas, params)
├── prompts/            # AI assistant prompts
├── data/               # Data storage
│   ├── universe.csv    # S&P 500 tickers
│   ├── prices/         # Price warehouse
│   └── compute_inputs/ # C++ input parquet
├── scraper/            # Python zone
├── cpp/                # C++ zone
├── out/                # Output artifacts
└── .github/workflows/  # CI/CD
```

## Quick Start

### Python Ingest
```bash
pip install -r requirements.txt
python -m scraper
```

### C++ Compute
```bash
cd cpp
mkdir build && cd build
cmake ..
make
./daily_run --input ../../data/compute_inputs/prices_window.parquet --output ../../out/daily/
```

## Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| `number_of_assets` | 500 | S&P 500 stocks |
| `rolling_window_length` | 50 | Trading days |
| `diffusion_eta` | TBD | Tune for ~0.7 avg smoothness |
| `strain_coefficients` | TBD | a, b, c, d weights |

## Strain Index Formula

```
Strain(t) = a * ||r_t||_2 + b * Sys(t) + c * Δ(t) + d * TP(t)
```

Where:
- `||r_t||_2` = L2 norm of returns (volatility magnitude)
- `Sys(t)` = Systemic ratio (smoothed vs raw returns)
- `Δ(t)` = Wasserstein distance (topology change)
- `TP(t)` = Total persistence (H1 features)

## Contracts

All cross-language communication follows strict contracts in `/contracts`:
- `params.md` - Canonical parameters
- `parquet_schema.md` - Data schemas
- `artifact_formats.md` - Binary layouts
- `math_spec.md` - Mathematical formulas

## License

[Add license here]
