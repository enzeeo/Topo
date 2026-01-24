#include "topology.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <hera/wasserstein.h>

// Embed CPU Ripser into this translation unit.
// We rename main to avoid symbol conflicts.
#define main ripser_embedded_main
#include "../third_party/ripser/ripser.cpp"
#undef main

static void validate_square_matrix_size_or_throw(
    const Matrix& matrix_values,
    uint32_t number_of_assets,
    const std::string& matrix_name
) {
    const size_t expected_element_count =
        static_cast<size_t>(number_of_assets) * static_cast<size_t>(number_of_assets);

    if (matrix_values.size() != expected_element_count) {
        throw std::runtime_error(
            matrix_name + " must have size N*N. Got size=" +
            std::to_string(matrix_values.size()) +
            ", expected=" + std::to_string(expected_element_count) +
            ", N=" + std::to_string(number_of_assets)
        );
    }
}

static void validate_finite_or_throw(double value, const std::string& name) {
    if (std::isnan(value) || std::isinf(value)) {
        throw std::runtime_error(name + " must be finite (not NaN/Inf)");
    }
}

Matrix correlation_to_distance(
    const Matrix& correlation_matrix,
    uint32_t number_of_assets
) {
    validate_square_matrix_size_or_throw(
        correlation_matrix,
        number_of_assets,
        "correlation_matrix"
    );

    Matrix distance_matrix(
        static_cast<size_t>(number_of_assets) * static_cast<size_t>(number_of_assets),
        0.0
    );

    for (uint32_t i = 0; i < number_of_assets; ++i) {
        for (uint32_t j = 0; j < number_of_assets; ++j) {
            const size_t flat_index =
                static_cast<size_t>(i) * static_cast<size_t>(number_of_assets) +
                static_cast<size_t>(j);

            const double correlation_value = correlation_matrix[flat_index];
            validate_finite_or_throw(correlation_value, "correlation_matrix entry");

            // Clamp to [-1, 1] to avoid sqrt of small negative values due to numeric drift.
            const double clamped_correlation =
                std::min(1.0, std::max(-1.0, correlation_value));

            // d_{ij} = sqrt(2 * (1 - corr))
            const double one_minus_correlation = 1.0 - clamped_correlation;
            const double inside_sqrt = 2.0 * one_minus_correlation;
            const double clamped_inside_sqrt = std::max(0.0, inside_sqrt);

            distance_matrix[flat_index] = std::sqrt(clamped_inside_sqrt);
        }
    }

    // Enforce exact zeros on the diagonal.
    for (uint32_t i = 0; i < number_of_assets; ++i) {
        distance_matrix[
            static_cast<size_t>(i) * static_cast<size_t>(number_of_assets) +
            static_cast<size_t>(i)
        ] = 0.0;
    }

    return distance_matrix;
}

PersistenceDiagram compute_persistence_H1(
    const Matrix& distance_matrix,
    uint32_t number_of_assets
) {
    validate_square_matrix_size_or_throw(distance_matrix, number_of_assets, "distance_matrix");

    const size_t number_of_points = static_cast<size_t>(number_of_assets);
    const size_t number_of_lower_triangle_entries =
        (number_of_points * (number_of_points - 1)) / 2;

    std::vector<value_t> lower_triangle_distances;
    lower_triangle_distances.reserve(number_of_lower_triangle_entries);

    for (uint32_t i = 1; i < number_of_assets; ++i) {
        for (uint32_t j = 0; j < i; ++j) {
            const size_t flat_index =
                static_cast<size_t>(i) * static_cast<size_t>(number_of_assets) +
                static_cast<size_t>(j);

            const double distance_value = distance_matrix[flat_index];
            validate_finite_or_throw(distance_value, "distance_matrix entry");
            if (distance_value < 0.0) {
                throw std::runtime_error("distance_matrix must be non-negative");
            }

            lower_triangle_distances.push_back(static_cast<value_t>(distance_value));
        }
    }

    compressed_lower_distance_matrix ripser_distance_matrix(std::move(lower_triangle_distances));

    std::ostringstream captured_output;
    std::streambuf* previous_cout_buffer = std::cout.rdbuf(captured_output.rdbuf());

    try {
        const index_t dim_max = 1;
        const value_t threshold = std::numeric_limits<value_t>::max();
        const float ratio = 0.0f;
        const coefficient_t modulus = 2;

        ripser<compressed_lower_distance_matrix>(
            std::move(ripser_distance_matrix),
            dim_max,
            threshold,
            ratio,
            modulus
        ).compute_barcodes();
    } catch (...) {
        std::cout.rdbuf(previous_cout_buffer);
        throw;
    }

    std::cout.rdbuf(previous_cout_buffer);

    PersistenceDiagram diagram;

    std::istringstream output_lines(captured_output.str());
    std::string line;
    bool is_in_dim_1_section = false;

    while (std::getline(output_lines, line)) {
        if (line.find("persistence intervals in dim 1:") != std::string::npos) {
            is_in_dim_1_section = true;
            continue;
        }
        if (line.find("persistence intervals in dim ") != std::string::npos) {
            is_in_dim_1_section = false;
            continue;
        }
        if (!is_in_dim_1_section) {
            continue;
        }

        const size_t left_bracket = line.find('[');
        const size_t comma = line.find(',');
        const size_t right_paren = line.find(')');
        if (left_bracket == std::string::npos || comma == std::string::npos || right_paren == std::string::npos) {
            continue;
        }

        std::string birth_text = line.substr(left_bracket + 1, comma - (left_bracket + 1));
        std::string death_text = line.substr(comma + 1, right_paren - (comma + 1));

        auto trim_in_place = [](std::string& s) {
            const char* whitespace = " \t\n\r";
            const size_t first = s.find_first_not_of(whitespace);
            const size_t last = s.find_last_not_of(whitespace);
            if (first == std::string::npos) {
                s.clear();
                return;
            }
            s = s.substr(first, last - first + 1);
        };

        trim_in_place(birth_text);
        trim_in_place(death_text);

        if (birth_text.empty()) {
            continue;
        }
        if (death_text.empty()) {
            // Ignore infinite intervals for now.
            continue;
        }

        const double birth_value = std::stod(birth_text);
        const double death_value = std::stod(death_text);

        validate_finite_or_throw(birth_value, "diagram.birth");
        validate_finite_or_throw(death_value, "diagram.death");
        if (death_value < birth_value) {
            throw std::runtime_error("ripser output contains a pair with death < birth");
        }

        diagram.push_back(PersistencePair{birth_value, death_value});
    }

    return diagram;
}

double compute_total_persistence(const PersistenceDiagram& diagram) {
    double total_persistence = 0.0;

    for (const PersistencePair& pair : diagram) {
        validate_finite_or_throw(pair.birth, "diagram.birth");
        validate_finite_or_throw(pair.death, "diagram.death");

        const double lifetime = pair.death - pair.birth;
        if (lifetime < 0.0) {
            throw std::runtime_error("diagram contains a pair with death < birth");
        }

        total_persistence += lifetime;
    }

    return total_persistence;
}

void save_diagram_bin(
    const PersistenceDiagram& diagram,
    const std::string& output_path
) {
    std::ofstream output_file(output_path, std::ios::binary);
    if (!output_file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + output_path);
    }

    if (diagram.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
        throw std::runtime_error("diagram too large to serialize (exceeds uint32 pair count)");
    }

    const uint32_t number_of_pairs = static_cast<uint32_t>(diagram.size());
    output_file.write(reinterpret_cast<const char*>(&number_of_pairs), sizeof(uint32_t));

    for (const PersistencePair& pair : diagram) {
        validate_finite_or_throw(pair.birth, "diagram.birth");
        validate_finite_or_throw(pair.death, "diagram.death");

        output_file.write(reinterpret_cast<const char*>(&pair.birth), sizeof(double));
        output_file.write(reinterpret_cast<const char*>(&pair.death), sizeof(double));
    }

    if (!output_file.good()) {
        throw std::runtime_error("Error writing to file: " + output_path);
    }

    output_file.close();
}

PersistenceDiagram load_diagram_bin(const std::string& input_path) {
    std::ifstream input_file(input_path, std::ios::binary);
    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open file for reading: " + input_path);
    }

    uint32_t number_of_pairs = 0;
    input_file.read(reinterpret_cast<char*>(&number_of_pairs), sizeof(uint32_t));
    if (!input_file.good()) {
        throw std::runtime_error("Failed to read diagram header from: " + input_path);
    }

    PersistenceDiagram diagram;
    diagram.reserve(static_cast<size_t>(number_of_pairs));

    for (uint32_t pair_index = 0; pair_index < number_of_pairs; ++pair_index) {
        PersistencePair pair;

        input_file.read(reinterpret_cast<char*>(&pair.birth), sizeof(double));
        input_file.read(reinterpret_cast<char*>(&pair.death), sizeof(double));
        if (!input_file.good()) {
            throw std::runtime_error("Failed to read diagram pair from: " + input_path);
        }

        validate_finite_or_throw(pair.birth, "diagram.birth");
        validate_finite_or_throw(pair.death, "diagram.death");

        if (pair.death < pair.birth) {
            throw std::runtime_error("diagram contains a pair with death < birth");
        }

        diagram.push_back(pair);
    }

    return diagram;
}

double compute_wasserstein_distance(
    const PersistenceDiagram& current_diagram,
    const PersistenceDiagram& previous_diagram
) {
    std::vector<std::pair<double, double>> current_pairs;
    current_pairs.reserve(current_diagram.size());

    for (const PersistencePair& pair : current_diagram) {
        validate_finite_or_throw(pair.birth, "current_diagram.birth");
        validate_finite_or_throw(pair.death, "current_diagram.death");
        if (pair.death < pair.birth) {
            throw std::runtime_error("current_diagram contains a pair with death < birth");
        }
        current_pairs.emplace_back(pair.birth, pair.death);
    }

    std::vector<std::pair<double, double>> previous_pairs;
    previous_pairs.reserve(previous_diagram.size());

    for (const PersistencePair& pair : previous_diagram) {
        validate_finite_or_throw(pair.birth, "previous_diagram.birth");
        validate_finite_or_throw(pair.death, "previous_diagram.death");
        if (pair.death < pair.birth) {
            throw std::runtime_error("previous_diagram contains a pair with death < birth");
        }
        previous_pairs.emplace_back(pair.birth, pair.death);
    }

    hera::AuctionParams<double> params;
    params.wasserstein_power = 1.0;
    params.internal_p = 2.0;

    const double distance_value = hera::wasserstein_dist(current_pairs, previous_pairs, params);
    validate_finite_or_throw(distance_value, "wasserstein_distance");
    return distance_value;
}
