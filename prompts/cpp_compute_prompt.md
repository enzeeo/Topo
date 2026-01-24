# C++ Compute Prompt

## Scope (C++ ONLY)

You implement the numerical and topological computation.

You do NOT:

- Fetch market data
- Modify parquet schemas
- Perform ingestion validation

---

## Readability Requirements

- All variable names must be descriptive English words.
- Avoid single-letter variables except:
  - i, j for matrix indices
- Break long formulas into named steps.
- Write loops explicitly.
- No clever STL tricks unless required for performance.

---

## Allowed Optimizations

- Linear algebra routines (Eigen / BLAS)
- Parallel loops (OpenMP)
- Efficient matrix exponentiation

If optimizing:

- Add a comment explaining why it is necessary.

---

## Input

- `data/compute_inputs/prices_window.parquet`
- Guaranteed clean by contract

---

## Required Execution Order

DO NOT change this order.

1. `read_close_prices_parquet`
2. `compute_log_returns`
3. `save_returns_bin`
4. `compute_correlation_matrix`
5. `save_correlation_matrix_bin`
6. `build_weighted_adjacency_matrix`
7. `compute_graph_laplacian`
8. `compute_graph_total_variation`
9. `diffuse_return_vectors`
10. `compute_systemic_ratio`
11. `convert_correlation_to_distance`
12. `compute_persistent_homology_H1`
13. `save_persistence_diagram_bin`
14. `compute_total_persistence`
15. `load_previous_persistence_diagram`
16. `compute_wasserstein_distance`
17. `compute_strain_index`
18. `write_strain_json`

---

## Output

- `out/daily/date=YYYY-MM-DD/`
  - strain.json
  - returns.bin
  - corr.bin
  - diagram.bin

---

## Determinism Rules

- Fixed ordering
- No randomness
- No non-deterministic threading behavior

---

## Goal

Produce a clear, traceable, mathematically faithful strain index.
