# Contracts

This directory defines immutable contracts between Python and C++ components.

## Files

- **params.md** - Canonical parameters (n=500, m=50, eta, strain coefficients)
- **parquet_schema.md** - Schema definitions for prices.parquet and prices_window.parquet
- **artifact_formats.md** - Binary artifact layouts (returns.bin, corr.bin, diagram.bin)
- **math_spec.md** - Mathematical formulas and invariants from the paper

## Rules

1. Code must conform to contracts exactly
2. If code and contract disagree, **contract wins**
3. Do NOT change contracts unless explicitly instructed
4. Any contract change requires:
   - Contract doc update FIRST
   - Version bump note in README
   - Explicit migration plan
