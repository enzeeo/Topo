#!/usr/bin/env python3
"""
Recompute normalized_strain_index in existing out/daily strain.json files.

This avoids re-running the C++ pipeline. It updates only:
  normalized_strain_index = (strain_index - mean) / std_pop

Usage:
  python3 scripts/recompute_normalized_strain.py --out-daily out/daily --mean <m> --std-pop <s>
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


DATE_DIR_RE = re.compile(r"^date=(\d{4}-\d{2}-\d{2})$")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-daily", default="out/daily")
    ap.add_argument("--mean", type=float, required=True)
    ap.add_argument("--std-pop", type=float, required=True)
    args = ap.parse_args()

    if args.std_pop <= 0:
        raise SystemExit("--std-pop must be > 0")

    repo_root = Path(__file__).resolve().parents[1]
    out_daily = (repo_root / args.out_daily).resolve()
    if not out_daily.exists():
        raise SystemExit(f"Missing out/daily: {out_daily}")

    updated = 0
    skipped = 0

    for child in sorted(out_daily.iterdir()):
        if not child.is_dir():
            continue
        if not DATE_DIR_RE.match(child.name):
            continue
        p = child / "strain.json"
        if not p.exists():
            skipped += 1
            continue

        doc = json.loads(p.read_text(encoding="utf-8"))
        if "strain_index" not in doc:
            skipped += 1
            continue

        si = float(doc["strain_index"])
        doc["normalized_strain_index"] = (si - args.mean) / args.std_pop
        p.write_text(json.dumps(doc, indent=2, sort_keys=False) + "\n", encoding="utf-8")
        updated += 1

    print(f"updated={updated} skipped={skipped} out_daily={out_daily}")


if __name__ == "__main__":
    main()

