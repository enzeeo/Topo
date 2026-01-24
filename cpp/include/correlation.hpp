#pragma once

#include <string>
#include "types.hpp"

/**
 * @brief Compute correlation matrix from rolling returns.
 *
 * Purpose:
 *   Estimate asset-asset dependency structure over the window.
 *
 * Formula:
 *   M_{ij} = Cov(i,j) / sqrt(Var(i) * Var(j))
 *
 * @param returns Return matrix [m x N].
 * @param number_of_assets Number of assets (N).
 * @param window_length Window length (m).
 * @return Dense correlation matrix M(t), shape [N x N].
 *
 * Paper reference: Section 1-2 (correlation-based network)
 */
Matrix compute_correlation(
    const Matrix& returns,
    uint32_t number_of_assets,
    uint32_t window_length
);

/**
 * @brief Save correlation matrix to a binary file.
 *
 * Purpose:
 *   Persist correlation structure used by graph and topology stages.
 *
 * Binary layout:
 *   - uint32: number of assets (N)
 *   - double[N * N]: correlation matrix (row-major)
 *
 * @param correlation_matrix Correlation matrix to save.
 * @param number_of_assets Number of assets (N).
 * @param output_path Path to write the binary file.
 */
void save_correlation_bin(
    const Matrix& correlation_matrix,
    uint32_t number_of_assets,
    const std::string& output_path
);
