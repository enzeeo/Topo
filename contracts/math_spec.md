# Mathematical Specification

Reference: Algebraic Topology Strain Index paper

---

## 1. Log Returns

For stock i on day t:

```
r_{t,i} = log(P_{t,i} / P_{t-1,i})
```

Where `P_{t,i}` is the closing price of stock i on day t.

**Note**: Use natural log for additivity over time.

---

## 2. Correlation Matrix

For stocks i and j over rolling window of length m:

```
Covariance_{ij}(t) = (1/(m-1)) * Σ_{k=t-(m-1)}^{t} (r_{k,i} - r̄_i)(r_{k,j} - r̄_j)

Variance_i(t) = (1/(m-1)) * Σ_{k=t-(m-1)}^{t} (r_{k,i} - r̄_i)²

M_{ij}(t) = Covariance_{ij}(t) / sqrt(Variance_i(t) * Variance_j(t))
```

---

## 3. Weighted Graph

Convert correlation to weighted adjacency:

```
W_{ij}(t) = max(M_{ij}(t), 0)  for i ≠ j
W_{ii}(t) = 0                   for all i
```

**Interpretation**: Positive correlation → weighted edge; negative correlation → no edge.

---

## 4. Graph Laplacian

Degree of stock i:
```
d_i(t) = Σ_{k=1}^{n} W_{ik}(t)
```

Degree matrix:
```
D_{ii}(t) = d_i(t)
D_{ij}(t) = 0  for i ≠ j
```

Laplacian:
```
L(t) = D(t) - W(t)
```

---

## 5. Graph Total Variation

```
GTV(t) = r_t^T L(t) r_t
```

**Interpretation**: Small GTV means correlated stocks move similarly.

---

## 6. Laplacian Diffusion

Diffusion operator:
```
S(t) = exp(-η * L(t))
```

Smoothed returns:
```
s_t = S(t) * r_t = exp(-η * L(t)) * r_t
```

**Parameter η**: Tune so historical average smoothness ≈ 0.7

Smoothness:
```
Smoothness_η(t) = ||s_t||_2 / ||r_t||_2
```

---

## 7. Systemic Ratio

```
Sys(t) = ||s_t||_2 / ||r_t||_2
```

**Interpretation**: High Sys means market-wide movement dominates.

---

## 8. Correlation to Distance

```
d_{ij}(t) = sqrt(2 * (1 - M_{ij}(t)))
```

**Properties**:
- d_{ij} = 0 when M_{ij} = 1 (perfect correlation)
- d_{ij} = sqrt(2) ≈ 1.414 when M_{ij} = 0 (no correlation)
- d_{ij} = 2 when M_{ij} = -1 (perfect anti-correlation)

---

## 9. Persistent Homology (H1)

Track H1 topological features (loops) using distance threshold ε.

For each loop i:
- `b_i`: birth time (ε at which loop forms)
- `d_i`: death time (ε at which loop fills in)
- Lifetime: `d_i - b_i`

Persistence diagram:
```
D(t) = {(b_i(t), d_i(t))}
```

---

## 10. Total Persistence

```
TP(t) = Σ_i (d_i - b_i)
```

---

## 11. Diagram Distance

Wasserstein distance between consecutive days:

```
Δ(t) = W(D(t), D(t-1))
```

Where W is the Wasserstein distance (preferred over bottleneck distance).

---

## 12. Strain Index

Final combined index:

```
Strain(t) = a * sqrt(Σ_{i=1}^{n} r_{t,i}²) + b * Sys(t) + c * Δ(t) + d * TP(t)
```

Or equivalently:
```
Strain(t) = a * ||r_t||_2 + b * Sys(t) + c * Δ(t) + d * TP(t)
```

Where a, b, c, d are hyperparameters to be tuned.
