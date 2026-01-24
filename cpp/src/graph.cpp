#include "graph.hpp"

Matrix build_weighted_graph(
    const Matrix& correlation_matrix,
    uint32_t number_of_assets
) {
    // TODO: Implement W_{ij} = max(M_{ij}, 0) for i != j
    throw std::runtime_error("Not implemented");
}

Matrix compute_laplacian(
    const Matrix& weighted_adjacency,
    uint32_t number_of_assets
) {
    // TODO: Implement L = D - W
    throw std::runtime_error("Not implemented");
}

double compute_graph_total_variation(
    const Vector& latest_return,
    const Matrix& laplacian,
    uint32_t number_of_assets
) {
    // TODO: Implement GTV = r_t^T L r_t
    throw std::runtime_error("Not implemented");
}
