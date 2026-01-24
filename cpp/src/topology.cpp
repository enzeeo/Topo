#include "topology.hpp"

Matrix correlation_to_distance(
    const Matrix& correlation_matrix,
    uint32_t number_of_assets
) {
    // TODO: Implement d_{ij} = sqrt(2 * (1 - M_{ij}))
    throw std::runtime_error("Not implemented");
}

PersistenceDiagram compute_persistence_H1(
    const Matrix& distance_matrix,
    uint32_t number_of_assets
) {
    // TODO: Implement using ripser++ library
    throw std::runtime_error("Not implemented");
}

double compute_total_persistence(const PersistenceDiagram& diagram) {
    // TODO: Implement TP = sum_i (death_i - birth_i)
    throw std::runtime_error("Not implemented");
}

void save_diagram_bin(
    const PersistenceDiagram& diagram,
    const std::string& output_path
) {
    // TODO: Implement binary serialization
    throw std::runtime_error("Not implemented");
}

PersistenceDiagram load_diagram_bin(const std::string& input_path) {
    // TODO: Implement binary deserialization
    throw std::runtime_error("Not implemented");
}

double compute_wasserstein_distance(
    const PersistenceDiagram& current_diagram,
    const PersistenceDiagram& previous_diagram
) {
    // TODO: Implement using Hera library
    throw std::runtime_error("Not implemented");
}
