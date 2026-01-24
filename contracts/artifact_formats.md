# Binary Artifact Formats

All binary files use little-endian byte order.

---

## `returns.bin`

Rolling-window return matrix.

| Offset | Type | Description |
|--------|------|-------------|
| 0 | uint32 | `number_of_assets` (N) |
| 4 | uint32 | `rolling_window_length` (m, number of return rows) |
| 8 | double[m * N] | Return matrix (m return rows), row-major |

**Total size**: 8 + (m * N * 8) bytes

---

## `corr.bin`

Correlation matrix.

| Offset | Type | Description |
|--------|------|-------------|
| 0 | uint32 | `number_of_assets` (N) |
| 4 | double[N * N] | Correlation matrix, row-major |

**Total size**: 4 + (N * N * 8) bytes

---

## `diagram.bin`

Persistence diagram (H1 features).

| Offset | Type | Description |
|--------|------|-------------|
| 0 | uint32 | `number_of_pairs` |
| 4 | sequence | Pairs of (double birth, double death) |

**Per pair**: 16 bytes (2 × double)

**Total size**: 4 + (number_of_pairs * 16) bytes

---

## `strain.json`

Final output in JSON format.

```json
{
  "date": "YYYY-MM-DD",
  "l2_return_magnitude": <double>,
  "graph_total_variation": <double>,
  "systemic_ratio": <double>,
  "total_persistence": <double>,
  "wasserstein_distance": <double>,
  "strain_index": <double>
}
```

**Rules**:
- All numeric fields are IEEE 754 double precision
- Date format is ISO 8601 (YYYY-MM-DD)
- No additional fields allowed
