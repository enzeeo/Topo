"""
Functions for building and writing QC reports.
"""

import json
from datetime import date
from pathlib import Path
from typing import List, Optional

from .config import QCReport, Universe


def build_qc_report(
    run_date: date,
    fetched_dates: List[date],
    universe: Universe,
    missing_tickers: List[str],
    duplicate_rows: int,
    bad_value_rows: int,
    notes: Optional[List[str]] = None,
) -> QCReport:
    """
    Construct a QC report for the run.

    Args:
        run_date: Logical market date targeted.
        fetched_dates: Dates covered by the fetch.
        universe: Universe metadata.
        missing_tickers: Tickers missing data for run_date.
        duplicate_rows: Number of duplicates removed/detected.
        bad_value_rows: Number of invalid-value rows detected.
        notes: Optional warnings/notes.

    Returns:
        QCReport object.
    """
    if notes is None:
        notes_list = []
    else:
        notes_list = notes
    
    number_of_tickers = len(universe.tickers)
    
    qc_report = QCReport(
        run_date=run_date,
        fetched_dates=fetched_dates,
        n_tickers=number_of_tickers,
        missing_tickers=missing_tickers,
        duplicate_rows=duplicate_rows,
        bad_value_rows=bad_value_rows,
        notes=notes_list,
    )
    
    return qc_report


def write_qc_report_json(
    qc: QCReport,
    qc_out_dir: Path,
) -> Path:
    """
    Write the QC report as JSON under out/qc/date=YYYY-MM-DD/.

    Args:
        qc: QCReport object.
        qc_out_dir: Root output directory for QC artifacts.

    Returns:
        Path to the written JSON file.
    """
    date_string = qc.run_date.strftime("%Y-%m-%d")
    date_directory = qc_out_dir / f"date={date_string}"
    date_directory.mkdir(parents=True, exist_ok=True)
    
    json_file_path = date_directory / "qc_report.json"
    
    report_dict = {
        "run_date": qc.run_date.isoformat(),
        "fetched_dates": [single_date.isoformat() for single_date in qc.fetched_dates],
        "n_tickers": qc.n_tickers,
        "missing_tickers": qc.missing_tickers,
        "duplicate_rows": qc.duplicate_rows,
        "bad_value_rows": qc.bad_value_rows,
        "notes": qc.notes,
    }
    
    with open(json_file_path, "w") as json_file:
        json.dump(report_dict, json_file, indent=2)
    
    return json_file_path
