#!/usr/bin/env python3

"""
Tune strain coefficients to forecast future drawdown risk.

Purpose
-------
The Market Strain Index is intended as a structural fragility / systemic-risk signal.
To align coefficients with that purpose, we tune (a,b,c,d,e) so a linear score
predicts SPY's *future maximum drawdown* over a horizon (default: 10 trading days).

This script DOES NOT recompute topology. It uses your existing daily outputs:
  out/daily/date=*/strain.json

Input features (X_t)
--------------------
From strain.json at date t:
  - l2_return_magnitude
  - systemic_ratio
  - wasserstein_distance
  - total_persistence
  - graph_total_variation

Target (y_t)
------------
Future max drawdown of SPY over [t+1, t+h] trading days:

  Let prices = SPY Adj Close.
  Consider the forward window prices p_{t+1}, ..., p_{t+h}.
  Define running peak within that window:
      peak_k = max(p_{t+1}, ..., p_{t+k})
  Define drawdown at k:
      dd_k = (peak_k - p_{t+k}) / peak_k
  Then:
      y_t = max_k dd_k

Model
-----
Ridge regression on standardized features:
  y_hat = intercept + w · z(X_t)
We output equivalent coefficients in the *original* component units, so you can
use them directly as daily_run weights:

  strain_index = a*l2 + b*sys + c*wass + d*tp + e*gtv

Note: because we standardize features for fitting, the returned (a..e) are
converted back to original units.

No-lookahead
------------
- Uses features at date t and a target computed from future SPY prices.
- Suggested split is time-based train/test.

Dependencies
------------
Requires: pandas, numpy, yfinance
"""

from __future__ import annotations

import argparse
import json
import math
import os
import re
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Tuple

import numpy as np
import pandas as pd
import yfinance as yf


DATE_DIR_RE = re.compile(r"^date=(\d{4}-\d{2}-\d{2})$")


FEATURE_KEYS: Tuple[str, ...] = (
    "l2_return_magnitude",
    "systemic_ratio",
    "wasserstein_distance",
    "total_persistence",
    "graph_total_variation",
)


def _is_finite(x: float) -> bool:
    return not (math.isnan(x) or math.isinf(x))


@dataclass(frozen=True)
class FeatureRow:
    date: pd.Timestamp
    values: Dict[str, float]


def load_feature_rows(out_daily_dir: Path) -> List[FeatureRow]:
    rows: List[FeatureRow] = []

    if not out_daily_dir.exists():
        raise RuntimeError(f"Missing out/daily dir: {out_daily_dir}")

    for child in out_daily_dir.iterdir():
        if not child.is_dir():
            continue
        m = DATE_DIR_RE.match(child.name)
        if m is None:
            continue
        date_str = m.group(1)
        p = child / "strain.json"
        if not p.exists():
            continue

        with open(p, "r", encoding="utf-8") as f:
            doc = json.load(f)

        vals: Dict[str, float] = {}
        for k in FEATURE_KEYS:
            if k not in doc:
                raise RuntimeError(f"Missing key '{k}' in {p}")
            v = float(doc[k])
            if not _is_finite(v):
                raise RuntimeError(f"Non-finite {k}={v} in {p}")
            vals[k] = v

        rows.append(FeatureRow(date=pd.to_datetime(date_str), values=vals))

    rows.sort(key=lambda r: r.date)
    if not rows:
        raise RuntimeError(f"No strain.json rows found under {out_daily_dir}/date=*/strain.json")
    return rows


def download_spy_adjclose(start: pd.Timestamp, end: pd.Timestamp) -> pd.Series:
    """
    Download SPY adjusted close.

    yfinance end is inclusive-ish; we request a bit extra and then slice.
    """
    df = yf.download("SPY", start=start.date(), end=(end + pd.Timedelta(days=7)).date(), progress=False)
    if df is None or df.empty:
        raise RuntimeError("Failed to download SPY data via yfinance (empty result)")
    if "Adj Close" not in df.columns:
        raise RuntimeError("SPY download missing 'Adj Close' column")
    s = df["Adj Close"].copy()
    s.index = pd.to_datetime(s.index)
    s = s.sort_index()
    s = s.loc[(s.index >= start) & (s.index <= end)]
    if s.empty:
        raise RuntimeError("SPY series empty after date slicing")
    return s


def forward_max_drawdown(prices: np.ndarray) -> float:
    """
    prices: shape (h,) for t+1..t+h
    returns max drawdown in [0, 1).
    """
    peak = prices[0]
    max_dd = 0.0
    for p in prices:
        if p > peak:
            peak = p
        dd = (peak - p) / peak
        if dd > max_dd:
            max_dd = dd
    return float(max_dd)


def build_dataset(
    feature_rows: List[FeatureRow],
    spy: pd.Series,
    horizon_days: int,
) -> Tuple[pd.DatetimeIndex, np.ndarray, np.ndarray]:
    """
    Align features with SPY dates and compute forward max drawdown targets.
    Returns (dates, X, y) where X shape (n, 5), y shape (n,).
    """
    # Align on dates present in both sources
    feature_dates = pd.DatetimeIndex([r.date.normalize() for r in feature_rows])
    spy_dates = pd.DatetimeIndex(pd.to_datetime(spy.index).normalize())

    # Build a date->row map for features
    by_date: Dict[pd.Timestamp, FeatureRow] = {r.date.normalize(): r for r in feature_rows}

    common_dates = feature_dates.intersection(spy_dates).sort_values()
    if len(common_dates) < (horizon_days + 5):
        raise RuntimeError("Not enough overlapping dates between out/daily and SPY prices")

    # For each date t, we need future t+1..t+h available in SPY
    spy_by_date = spy.copy()
    spy_by_date.index = pd.to_datetime(spy_by_date.index).normalize()
    spy_by_date = spy_by_date[~spy_by_date.index.duplicated(keep="last")].sort_index()

    X_list: List[List[float]] = []
    y_list: List[float] = []
    dates_out: List[pd.Timestamp] = []

    for t in common_dates:
        # Build forward window trading days from SPY index
        idx_pos = spy_by_date.index.get_indexer([t])[0]
        if idx_pos < 0:
            continue
        start_pos = idx_pos + 1
        end_pos = idx_pos + 1 + horizon_days
        if end_pos > len(spy_by_date.index):
            break

        window = spy_by_date.iloc[start_pos:end_pos].to_numpy(dtype=float)
        if window.shape[0] != horizon_days:
            continue
        if np.any(~np.isfinite(window)) or np.any(window <= 0):
            continue

        y = forward_max_drawdown(window)
        fr = by_date[t]
        x = [fr.values[k] for k in FEATURE_KEYS]
        if not all(np.isfinite(xi) for xi in x):
            continue

        dates_out.append(t)
        X_list.append(x)
        y_list.append(y)

    if len(dates_out) < 50:
        raise RuntimeError("Too few usable rows after alignment/target construction")

    X = np.asarray(X_list, dtype=float)
    y = np.asarray(y_list, dtype=float)
    return pd.DatetimeIndex(dates_out), X, y


def ridge_fit_standardized(X_train: np.ndarray, y_train: np.ndarray, ridge_alpha: float):
    """
    Fit ridge regression with an intercept, with standardization on X.
    Returns: (intercept, coef_original_units, feature_means, feature_stds)
    """
    if ridge_alpha < 0:
        raise ValueError("ridge_alpha must be >= 0")

    mu = X_train.mean(axis=0)
    sigma = X_train.std(axis=0, ddof=0)
    sigma = np.where(sigma <= 1e-12, 1.0, sigma)

    Z = (X_train - mu) / sigma

    # Add intercept column
    ones = np.ones((Z.shape[0], 1), dtype=float)
    A = np.concatenate([ones, Z], axis=1)

    # Ridge penalty on weights only, not intercept
    p = A.shape[1]
    I = np.eye(p, dtype=float)
    I[0, 0] = 0.0

    # Solve (A^T A + alpha I) w = A^T y
    lhs = A.T @ A + ridge_alpha * I
    rhs = A.T @ y_train
    w = np.linalg.solve(lhs, rhs)

    intercept = float(w[0])
    w_z = w[1:]
    # Convert back to original units: y = intercept + sum_j (w_zj * (xj - muj)/sigj)
    # => y = (intercept - sum_j w_zj*muj/sigj) + sum_j (w_zj/sigj) * xj
    coef_orig = (w_z / sigma).astype(float)
    intercept_orig = float(intercept - np.sum(w_z * (mu / sigma)))

    return intercept_orig, coef_orig, mu, sigma


def evaluate_regression(y_true: np.ndarray, y_pred: np.ndarray) -> Dict[str, float]:
    err = y_pred - y_true
    mse = float(np.mean(err ** 2))
    rmse = float(np.sqrt(mse))
    mae = float(np.mean(np.abs(err)))
    # R^2
    denom = float(np.sum((y_true - y_true.mean()) ** 2))
    r2 = float(1.0 - (np.sum(err ** 2) / denom)) if denom > 0 else float("nan")
    return {"rmse": rmse, "mae": mae, "r2": r2}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-daily", default="out/daily")
    parser.add_argument("--horizon-days", type=int, default=10)
    parser.add_argument("--ridge-alpha", type=float, default=10.0, help="Ridge regularization strength (>=0).")
    parser.add_argument(
        "--train-end",
        default="2025-06-30",
        help="Inclusive end date for training split (YYYY-MM-DD). Test starts next trading day.",
    )
    parser.add_argument(
        "--start-date",
        default=None,
        help="Optional earliest date to include (YYYY-MM-DD).",
    )
    parser.add_argument(
        "--end-date",
        default=None,
        help="Optional latest date to include (YYYY-MM-DD).",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    out_daily_dir = (repo_root / args.out_daily).resolve()

    feature_rows = load_feature_rows(out_daily_dir)

    if args.start_date:
        start_dt = pd.to_datetime(args.start_date)
        feature_rows = [r for r in feature_rows if r.date >= start_dt]
    if args.end_date:
        end_dt = pd.to_datetime(args.end_date)
        feature_rows = [r for r in feature_rows if r.date <= end_dt]

    min_date = feature_rows[0].date
    max_date = feature_rows[-1].date

    # Need extra forward days for target construction
    spy = download_spy_adjclose(min_date, max_date + pd.Timedelta(days=40))

    dates, X, y = build_dataset(feature_rows, spy, args.horizon_days)

    train_end = pd.to_datetime(args.train_end)
    train_mask = dates <= train_end
    test_mask = dates > train_end

    if train_mask.sum() < 50 or test_mask.sum() < 20:
        raise RuntimeError("Not enough rows in train/test split; adjust --train-end or date range.")

    X_train, y_train = X[train_mask], y[train_mask]
    X_test, y_test = X[test_mask], y[test_mask]

    intercept, coef_orig, mu, sigma = ridge_fit_standardized(X_train, y_train, args.ridge_alpha)

    yhat_train = intercept + X_train @ coef_orig
    yhat_test = intercept + X_test @ coef_orig

    train_metrics = evaluate_regression(y_train, yhat_train)
    test_metrics = evaluate_regression(y_test, yhat_test)

    print("## Tuned coefficients for future max drawdown")
    print(f"target=SPY_max_drawdown_next_{args.horizon_days}_days")
    print(f"train_end={args.train_end}")
    print(f"ridge_alpha={args.ridge_alpha}")
    print(f"rows_train={int(train_mask.sum())} rows_test={int(test_mask.sum())}")
    print("")
    print("train_metrics:", train_metrics)
    print("test_metrics:", test_metrics)
    print("")
    print("linear_model:")
    print(f"intercept={intercept}")
    # Map to daily_run coefficient names
    # strain_index = a*l2 + b*sys + c*wass + d*tp + e*gtv
    mapping = {
        "a": ("l2_return_magnitude", coef_orig[0]),
        "b": ("systemic_ratio", coef_orig[1]),
        "c": ("wasserstein_distance", coef_orig[2]),
        "d": ("total_persistence", coef_orig[3]),
        "e": ("graph_total_variation", coef_orig[4]),
    }
    for k, (feat, w) in mapping.items():
        print(f"{k} ({feat}) = {float(w)}")

    print("")
    print("ready_to_use_flags:")
    print(
        f"--a {float(coef_orig[0])} "
        f"--b {float(coef_orig[1])} "
        f"--c {float(coef_orig[2])} "
        f"--d {float(coef_orig[3])} "
        f"--e {float(coef_orig[4])}"
    )


if __name__ == "__main__":
    main()
