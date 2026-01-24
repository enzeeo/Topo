#!/usr/bin/env python3

import argparse
import os
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple


DATE_RE = re.compile(r"^prices_window_(\d{4}-\d{2}-\d{2})\.parquet$")


@dataclass(frozen=True)
class BackfillItem:
    run_date: str
    parquet_path: Path


def find_backfill_items(compute_inputs_dir: Path) -> List[BackfillItem]:
    items: List[BackfillItem] = []

    for child in compute_inputs_dir.iterdir():
        if not child.is_file():
            continue
        match = DATE_RE.match(child.name)
        if match is None:
            continue
        run_date = match.group(1)
        items.append(BackfillItem(run_date=run_date, parquet_path=child))

    items.sort(key=lambda x: x.run_date)
    return items


def filter_items_by_date_range(
    items: List[BackfillItem],
    start_date: Optional[str],
    end_date: Optional[str],
) -> List[BackfillItem]:
    filtered: List[BackfillItem] = []

    for item in items:
        if start_date is not None and item.run_date < start_date:
            continue
        if end_date is not None and item.run_date > end_date:
            continue
        filtered.append(item)

    return filtered


def ensure_daily_run_built(build_dir: Path, build_type: str, should_build: bool) -> Path:
    daily_run_path = build_dir / "daily_run"
    if daily_run_path.exists():
        return daily_run_path

    if not should_build:
        raise RuntimeError(
            f"Missing {daily_run_path}. Build first, or pass --build.\n"
            f"Example:\n"
            f"  cmake -S cpp -B {build_dir} -DCMAKE_BUILD_TYPE={build_type}\n"
            f"  cmake --build {build_dir} --target daily_run\n"
        )

    repo_root = Path(__file__).resolve().parents[1]
    cmake_configure = [
        "cmake",
        "-S",
        str(repo_root / "cpp"),
        "-B",
        str(build_dir),
        f"-DCMAKE_BUILD_TYPE={build_type}",
    ]
    cmake_build = [
        "cmake",
        "--build",
        str(build_dir),
        "--target",
        "daily_run",
    ]

    subprocess.run(cmake_configure, check=True)
    subprocess.run(cmake_build, check=True)

    if not daily_run_path.exists():
        raise RuntimeError(f"Build completed but missing {daily_run_path}")

    return daily_run_path


def run_one(
    daily_run_path: Path,
    parquet_path: Path,
    output_root: Path,
    run_date: str,
    eta: float,
    a: float,
    b: float,
    c: float,
    d: float,
    extra_args: List[str],
    env: dict,
    dry_run: bool,
) -> None:
    command: List[str] = [
        str(daily_run_path),
        "--input",
        str(parquet_path),
        "--output",
        str(output_root),
        "--date",
        run_date,
        "--eta",
        str(eta),
        "--a",
        str(a),
        "--b",
        str(b),
        "--c",
        str(c),
        "--d",
        str(d),
    ]
    command.extend(extra_args)

    if dry_run:
        print(" ".join(command))
        return

    subprocess.run(command, check=True, env=env)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--compute-inputs-dir",
        default="data/compute_inputs",
        help="Directory containing prices_window_YYYY-MM-DD.parquet files.",
    )
    parser.add_argument(
        "--build-dir",
        default="cpp/build",
        help="CMake build dir containing daily_run.",
    )
    parser.add_argument(
        "--build-type",
        default="Release",
        choices=["Release", "Debug", "RelWithDebInfo", "MinSizeRel"],
        help="CMake build type used if --build is set.",
    )
    parser.add_argument(
        "--build",
        action="store_true",
        help="If set, run cmake configure/build if daily_run is missing.",
    )
    parser.add_argument(
        "--output-root",
        default="out/daily",
        help="Output root where out/daily/date=YYYY-MM-DD/ is created.",
    )
    parser.add_argument("--start-date", default=None, help="Inclusive YYYY-MM-DD filter.")
    parser.add_argument("--end-date", default=None, help="Inclusive YYYY-MM-DD filter.")
    parser.add_argument("--dry-run", action="store_true", help="Print commands only.")

    parser.add_argument("--eta", type=float, default=1.0)
    parser.add_argument("--a", type=float, default=1.0)
    parser.add_argument("--b", type=float, default=1.0)
    parser.add_argument("--c", type=float, default=1.0)
    parser.add_argument("--d", type=float, default=1.0)

    parser.add_argument(
        "--extra-arg",
        action="append",
        default=[],
        help="Extra argument to forward to daily_run (repeatable).",
    )

    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    compute_inputs_dir = (repo_root / args.compute_inputs_dir).resolve()
    output_root = (repo_root / args.output_root).resolve()
    build_dir = (repo_root / args.build_dir).resolve()

    items = find_backfill_items(compute_inputs_dir)
    if not items:
        raise RuntimeError(f"No inputs found in {compute_inputs_dir} matching prices_window_YYYY-MM-DD.parquet")

    filtered = filter_items_by_date_range(items, args.start_date, args.end_date)
    if not filtered:
        raise RuntimeError("No inputs matched date range filters")

    daily_run_path = ensure_daily_run_built(build_dir, args.build_type, args.build)

    env = dict(os.environ)
    env.pop("RUN_DATE", None)

    for item in filtered:
        run_one(
            daily_run_path=daily_run_path,
            parquet_path=item.parquet_path,
            output_root=output_root,
            run_date=item.run_date,
            eta=args.eta,
            a=args.a,
            b=args.b,
            c=args.c,
            d=args.d,
            extra_args=args.extra_arg,
            env=env,
            dry_run=args.dry_run,
        )


if __name__ == "__main__":
    main()

