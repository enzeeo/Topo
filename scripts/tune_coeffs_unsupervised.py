#!/usr/bin/env python3

"""
Unsupervised coefficient "tuning" for the strain equation.

This script does NOT use an external target (VIX, drawdowns, etc.). Instead, it:
  - reads historical daily component outputs (out/daily/date=*/strain.json)
  - computes a scale estimate per component
  - recommends coefficients that approximately equalize typical contribution sizes

Recommended formula (C++ daily_run):
  Strain = a*L2 + b*Sys + c*Wass + d*TP + e*GTV

By default, coefficients are:
  coef_i = weight_i / scale_i

Where scale_i is the sample standard deviation of the component series.
Optionally, use a robust scale (MAD) via --scale mad.
"""

from __future__ import annotations

import argparse
import glob
import json
import math
import os
import statistics
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple


@dataclass(frozen=True)
class Series:
    name: str
    values: List[float]


COMPONENT_KEYS: Tuple[str, ...] = (
    "l2_return_magnitude",
    "systemic_ratio",
    "wasserstein_distance",
    "total_persistence",
    "graph_total_variation",
)


def is_finite(x: float) -> bool:
    return not (math.isnan(x) or math.isinf(x))


def median_abs_deviation(values: List[float]) -> float:
    # MAD = median(|x - median(x)|). No consistency constant by default.
    m = statistics.median(values)
    deviations = [abs(v - m) for v in values]
    return statistics.median(deviations)


def load_components_from_out(out_daily_dir: Path) -> Dict[str, Series]:
    series: Dict[str, List[float]] = {k: [] for k in COMPONENT_KEYS}

    pattern = str(out_daily_dir / "date=*/strain.json")
    paths = sorted(glob.glob(pattern))
    if not paths:
        raise RuntimeError(f"No strain.json files found at {pattern}")

    for p in paths:
        with open(p, "r", encoding="utf-8") as f:
            doc = json.load(f)

        for key in COMPONENT_KEYS:
            if key not in doc:
                raise RuntimeError(f"Missing key '{key}' in {p}")
            value = float(doc[key])
            if not is_finite(value):
                raise RuntimeError(f"Non-finite {key}={value} in {p}")
            series[key].append(value)

    return {k: Series(name=k, values=v) for k, v in series.items()}


def compute_scale(values: List[float], method: str) -> float:
    if len(values) < 2:
        raise RuntimeError("Need at least 2 samples to compute scale")

    if method == "std":
        s = statistics.pstdev(values) if len(values) == 2 else statistics.stdev(values)
        return float(s)

    if method == "mad":
        mad = median_abs_deviation(values)
        return float(mad)

    raise RuntimeError(f"Unknown scale method: {method}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--out-daily",
        default="out/daily",
        help="Directory containing date=YYYY-MM-DD/strain.json outputs.",
    )
    parser.add_argument(
        "--scale",
        choices=["std", "mad"],
        default="std",
        help="Scale estimator used to normalize components.",
    )
    parser.add_argument(
        "--epsilon",
        type=float,
        default=1e-12,
        help="Lower bound to avoid division by zero.",
    )
    parser.add_argument("--weight-a", type=float, default=1.0)
    parser.add_argument("--weight-b", type=float, default=1.0)
    parser.add_argument("--weight-c", type=float, default=1.0)
    parser.add_argument("--weight-d", type=float, default=1.0)
    parser.add_argument("--weight-e", type=float, default=1.0)
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    out_daily_dir = (repo_root / args.out_daily).resolve()

    data = load_components_from_out(out_daily_dir)

    weights = {
        "l2_return_magnitude": float(args.weight_a),
        "systemic_ratio": float(args.weight_b),
        "wasserstein_distance": float(args.weight_c),
        "total_persistence": float(args.weight_d),
        "graph_total_variation": float(args.weight_e),
    }

    scales: Dict[str, float] = {}
    for key in COMPONENT_KEYS:
        scale = compute_scale(data[key].values, args.scale)
        if not is_finite(scale) or scale < 0.0:
            raise RuntimeError(f"Invalid scale for {key}: {scale}")
        scales[key] = max(scale, float(args.epsilon))

    coef_a = weights["l2_return_magnitude"] / scales["l2_return_magnitude"]
    coef_b = weights["systemic_ratio"] / scales["systemic_ratio"]
    coef_c = weights["wasserstein_distance"] / scales["wasserstein_distance"]
    coef_d = weights["total_persistence"] / scales["total_persistence"]
    coef_e = weights["graph_total_variation"] / scales["graph_total_variation"]

    print("## Unsupervised coefficient recommendation")
    print(f"scale_method={args.scale}")
    print("")
    for key in COMPONENT_KEYS:
        print(f"{key}_scale={scales[key]}")
    print("")
    print("coefficients:")
    print(f"a={coef_a}")
    print(f"b={coef_b}")
    print(f"c={coef_c}")
    print(f"d={coef_d}")
    print(f"e={coef_e}")
    print("")
    print("ready_to_use_flags:")
    print(f"--a {coef_a} --b {coef_b} --c {coef_c} --d {coef_d} --e {coef_e}")


if __name__ == "__main__":
    main()

