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
   - Materialize compute inputs:
     - `data/compute_inputs/prices_window_YYYY-MM-DD.parquet` (51 price rows)
     - `data/compute_inputs/prices_window.parquet` as a “latest” alias

2. **C++ Compute** (triggered after ingest)
   - Read dated price window parquet (`prices_window_YYYY-MM-DD.parquet`)
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
| `diffusion_eta` | `0.00536848` | Tuned so historical avg smoothness ≈ 0.7 |
| `strain_coefficients (a..e)` | `a=6.0790, b=7.9606, c=1.1955, d=0.1774, e=0.07336` | Tuned weights for strain components |

Notes:
- The `diffusion_eta` and `strain_coefficients (a..e)` above were tuned using **2022–2023 dates only** (train window). Treat 2024–2025 as out-of-sample evaluation when backtesting.
- Canonical values live in `contracts/params.md`.

## Strain Index Formula

```
Strain(t) = a * ||r_t||_2 + b * Sys(t) + c * Δ(t) + d * TP(t) + e * GTV(t)
```

Where:
- `||r_t||_2` = L2 norm of returns (volatility magnitude)
- `GTV(t)` = Graph total variation of returns on the correlation graph
- `Sys(t)` = Systemic ratio (smoothed vs raw returns)
- `Δ(t)` = Wasserstein distance (topology change)
- `TP(t)` = Total persistence (H1 features)

## Coefficient tuning (unsupervised)

If you don't have an external target (VIX, drawdowns, etc.), you can normalize component scales and choose weights for interpretability.

Run:
```bash
python3 scripts/tune_coeffs_unsupervised.py --out-daily out/daily --scale std
```

It prints recommended flags you can pass to `daily_run` / backfill:
```bash
--a <...> --b <...> --c <...> --d <...> --e <...>
```

## Contracts

All cross-language communication follows strict contracts in `/contracts`:
- `params.md` - Canonical parameters
- `parquet_schema.md` - Data schemas
- `artifact_formats.md` - Binary layouts
- `math_spec.md` - Mathematical formulas
