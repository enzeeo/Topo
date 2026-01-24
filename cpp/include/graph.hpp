#pragma once

#include "types.hpp"

/**
 * @brief Build weighted adjacency matrix from correlations.
 *
 * Purpose:
 *   Convert correlation matrix into a graph representation.
 *
 * Rule:
 *   W_{ij} = max(M_{ij}, 0) for i != j
 *   W_{ii} = 0 for all i
 *
 * @param correlation_matrix Correlation matrix [N x N].
 * @param number_of_assets Number of assets (N).
 * @return Weighted adjacency matrix W [N x N].
 *
 * Paper reference: Section 2 (graph construction strategy)
 */
Matrix build_weighted_graph(
    const Matrix& correlation_matrix,
    uint32_t number_of_assets
);

/**
 * @brief Compute graph Laplacian matrix.
 *
 * Purpose:
 *   Encode network structure for diffusion and smoothness analysis.
 *
 * Formula:
 *   L = D - W
 *   where D_{ii} = sum_k W_{ik} (degree matrix)
 *
 * @param weighted_adjacency Weighted adjacency matrix W [N x N].
 * @param number_of_assets Number of assets (N).
 * @return Graph Laplacian L [N x N].
 *
 * Paper reference: Section 3
 */
Matrix compute_laplacian(
    const Matrix& weighted_adjacency,
    uint32_t number_of_assets
);

/**
 * @brief Compute graph total variation (GTV).
 *
 * Purpose:
 *   Measure how misaligned returns are with network structure.
 *
 * Formula:
 *   GTV = r_t^T L r_t
 *
 * @param latest_return Latest return vector r_t [N].
 * @param laplacian Graph Laplacian L [N x N].
 * @param number_of_assets Number of assets (N).
 * @return Scalar GTV value.
 *
 * Paper reference: Section 3
 */
double compute_graph_total_variation(
    const Vector& latest_return,
    const Matrix& laplacian,
    uint32_t number_of_assets
);
