#pragma once

#include "types.hpp"

/**
 * @brief Diffuse returns over the market network.
 *
 * Purpose:
 *   Extract systemic (smooth) component of returns.
 *
 * Formula:
 *   s_t = exp(-eta * L) * r_t
 *
 * Implementation note:
 *   Uses symmetric eigendecomposition or Krylov expmv.
 *
 * @param laplacian Graph Laplacian L [N x N].
 * @param latest_return Latest return vector r_t [N].
 * @param number_of_assets Number of assets (N).
 * @param diffusion_eta Diffusion parameter eta.
 * @return Smoothed return vector s_t [N].
 *
 * Paper reference: Section 3
 */
Vector diffuse_returns(
    const Matrix& laplacian,
    const Vector& latest_return,
    uint32_t number_of_assets,
    double diffusion_eta
);

/**
 * @brief Compute systemic smoothness ratio.
 *
 * Purpose:
 *   Quantify dominance of systemic vs idiosyncratic movements.
 *
 * Formula:
 *   Sys(t) = ||s_t||_2 / ||r_t||_2
 *
 * @param smoothed_return Smoothed return vector s_t [N].
 * @param latest_return Latest return vector r_t [N].
 * @return Scalar systemic ratio.
 *
 * Paper reference: Section 3
 */
double compute_systemic_ratio(
    const Vector& smoothed_return,
    const Vector& latest_return
);
