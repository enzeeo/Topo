#include "strain.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <stdexcept>

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
    if (std::isnan(l2_return_magnitude) || std::isinf(l2_return_magnitude)) {
        throw std::runtime_error("l2_return_magnitude must be finite");
    }
    if (std::isnan(systemic_ratio) || std::isinf(systemic_ratio)) {
        throw std::runtime_error("systemic_ratio must be finite");
    }
    if (std::isnan(wasserstein_distance) || std::isinf(wasserstein_distance)) {
        throw std::runtime_error("wasserstein_distance must be finite");
    }
    if (std::isnan(total_persistence) || std::isinf(total_persistence)) {
        throw std::runtime_error("total_persistence must be finite");
    }

    if (l2_return_magnitude < 0.0) {
        throw std::runtime_error("l2_return_magnitude must be >= 0");
    }
    if (wasserstein_distance < 0.0) {
        throw std::runtime_error("wasserstein_distance must be >= 0");
    }
    if (total_persistence < 0.0) {
        throw std::runtime_error("total_persistence must be >= 0");
    }

    const double strain_index =
        coefficient_a * l2_return_magnitude +
        coefficient_b * systemic_ratio +
        coefficient_c * wasserstein_distance +
        coefficient_d * total_persistence;

    if (std::isnan(strain_index) || std::isinf(strain_index)) {
        throw std::runtime_error("strain_index computed as NaN/Inf");
    }

    return strain_index;
}

void write_strain_json(
    const StrainComponents& result,
    const std::string& output_path
) {
    std::ofstream output_file(output_path);
    if (!output_file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + output_path);
    }

    output_file << std::setprecision(17);

    output_file << "{\n";
    output_file << "  \"date\": \"" << result.date << "\",\n";
    output_file << "  \"l2_return_magnitude\": " << result.l2_return_magnitude << ",\n";
    output_file << "  \"graph_total_variation\": " << result.graph_total_variation << ",\n";
    output_file << "  \"systemic_ratio\": " << result.systemic_ratio << ",\n";
    output_file << "  \"total_persistence\": " << result.total_persistence << ",\n";
    output_file << "  \"wasserstein_distance\": " << result.wasserstein_distance << ",\n";
    output_file << "  \"strain_index\": " << result.strain_index << "\n";
    output_file << "}\n";

    if (!output_file.good()) {
        throw std::runtime_error("Error writing to file: " + output_path);
    }

    output_file.close();
}
