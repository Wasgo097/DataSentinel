import csv
import math
from dataclasses import dataclass
from pathlib import Path

import numpy as np


@dataclass(frozen=True)
class DatasetLoadResult:
    data: np.ndarray  # shape: (N, input_dim), dtype: float32
    dropped_rows: int


def _is_number(s: str) -> bool:
    try:
        float(s)
        return True
    except Exception:
        return False


def load_labeled_csv(path: str, input_dim: int = 8, normal_label: float = 0.0) -> DatasetLoadResult:
    """
    Load labeled training data from CSV.

    Expected format:
    - Each row has at least input_dim feature columns + 1 label column (last).
    - Optional header is allowed (auto-detected).
    - Training data is filtered to "normal" rows only (label == normal_label).
    """
    p = Path(path).expanduser().resolve()
    if not p.is_file():
        raise FileNotFoundError(f"Dataset file not found: {p}")

    with p.open("r", newline="") as f:
        reader = csv.reader(f)
        rows = [row for row in reader if row]

    if not rows:
        raise ValueError(f"Dataset file is empty: {p}")

    # Header if any of first input_dim cells is non-numeric.
    has_header = any((len(rows[0]) > i and not _is_number(rows[0][i])) for i in range(min(input_dim, len(rows[0]))))
    if has_header:
        rows = rows[1:]
        if not rows:
            raise ValueError(f"Dataset file has header but no data rows: {p}")

    dropped_rows = 0
    out: list[list[float]] = []

    for row in rows:
        # Strip and drop empty cells.
        row = [c.strip() for c in row if c is not None and c.strip() != ""]
        if len(row) < input_dim + 1:
            dropped_rows += 1
            continue

        try:
            feats = [float(row[i]) for i in range(input_dim)]
            label = float(row[-1])
        except Exception:
            dropped_rows += 1
            continue

        if any((not math.isfinite(v)) for v in feats) or not math.isfinite(label):
            dropped_rows += 1
            continue

        # Train only on normal rows.
        if label != float(normal_label):
            continue

        out.append(feats)

    if not out:
        raise ValueError(f"No valid normal rows loaded from: {p}")

    arr = np.asarray(out, dtype=np.float32)
    if arr.ndim != 2 or arr.shape[1] != input_dim:
        raise ValueError(f"Invalid dataset shape from {p}: expected (N,{input_dim}), got {arr.shape}")

    return DatasetLoadResult(data=arr, dropped_rows=dropped_rows)

