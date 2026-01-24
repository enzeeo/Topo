"""
Functions for building and writing QC reports.
"""

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
    raise NotImplementedError


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
    raise NotImplementedError
