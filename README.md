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
| `diffusion_eta` | `0.00245267` | Eta |
| `strain_coefficients (a..e)` | `a=0.00436939, b=0.0501097, c=-0.00153752, d=-0.000197485, e=-0.0000509529` | Tuned on 2022–2023 (train window) |

Notes:
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

## Fetching strain via HTTP (raw GitHub)

If the repo commits `out/daily/date=YYYY-MM-DD/strain.json`, you can fetch a day’s values directly:

```python
import requests

url = "https://raw.githubusercontent.com/enzeeo/Topo/main/out/daily/date=2026-01-23/strain.json"
data = requests.get(url, timeout=30).json()
print(data["strain_index"])
```
