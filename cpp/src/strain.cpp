#include "strain.hpp"

double compute_strain_index(
    double l2_return_magnitude,
    double systemic_ratio,
    double wasserstein_distance,
    double total_persistence,
    double coefficient_a,
    double coefficient_b,
    double coefficient_c,
    double coefficient_d
) {
    // TODO: Implement Strain = a * ||r_t||_2 + b * Sys + c * delta_W + d * TP
    throw std::runtime_error("Not implemented");
}

void write_strain_json(
    const StrainComponents& result,
    const std::string& output_path
) {
    // TODO: Implement JSON serialization
    throw std::runtime_error("Not implemented");
}
