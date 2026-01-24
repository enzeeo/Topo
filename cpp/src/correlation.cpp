#include "correlation.hpp"

Matrix compute_correlation(
    const Matrix& returns,
    uint32_t number_of_assets,
    uint32_t window_length
) {
    // TODO: Implement using Eigen/BLAS for efficiency
    throw std::runtime_error("Not implemented");
}

void save_correlation_bin(
    const Matrix& correlation_matrix,
    uint32_t number_of_assets,
    const std::string& output_path
) {
    // TODO: Implement binary serialization
    throw std::runtime_error("Not implemented");
}
