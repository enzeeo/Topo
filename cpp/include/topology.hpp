#pragma once

#include <string>
#include "types.hpp"

/**
 * @brief Convert correlation matrix to distance matrix.
 *
 * Purpose:
 *   Prepare metric space for persistent homology computation.
 *
 * Formula:
 *   d_{ij} = sqrt(2 * (1 - M_{ij}))
 *
 * @param correlation_matrix Correlation matrix M [N x N].
 * @param number_of_assets Number of assets (N).
 * @return Dense distance matrix [N x N].
 *
 * Paper reference: Section 4
 */
Matrix correlation_to_distance(
    const Matrix& correlation_matrix,
    uint32_t number_of_assets
);

/**
 * @brief Compute H1 persistence diagram from distance matrix.
 *
 * Purpose:
 *   Capture topological loops in market structure.
 *
 * @param distance_matrix Distance matrix [N x N].
 * @param number_of_assets Number of assets (N).
 * @return Persistence diagram (birth-death pairs).
 *
 * Paper reference: Section 4 (H1 features)
 */
PersistenceDiagram compute_persistence_H1(
    const Matrix& distance_matrix,
    uint32_t number_of_assets
);

/**
 * @brief Compute total persistence of a diagram.
 *
 * Purpose:
 *   Quantify overall strength of topological features.
 *
 * Formula:
 *   TP = sum_i (death_i - birth_i)
 *
 * @param diagram Persistence diagram.
 * @return Scalar total persistence.
 *
 * Paper reference: Section 4
 */
double compute_total_persistence(const PersistenceDiagram& diagram);

/**
 * @brief Save persistence diagram to a binary file.
 *
 * Purpose:
 *   Store topology snapshot for use in Wasserstein distance
 *   computation on the next trading day.
 *
 * Binary layout:
 *   - uint32: number of persistence pairs
 *   - repeated (double birth, double death)
 *
 * @param diagram Persistence diagram to save.
 * @param output_path Path to write the binary file.
 */
void save_diagram_bin(
    const PersistenceDiagram& diagram,
    const std::string& output_path
);

/**
 * @brief Load a persistence diagram from a binary file.
 *
 * Purpose:
 *   Retrieve previous day's topological structure for
 *   Wasserstein distance computation.
 *
 * @param input_path Path to the binary file.
 * @return Persistence diagram (birth-death pairs).
 */
PersistenceDiagram load_diagram_bin(const std::string& input_path);

/**
 * @brief Compute Wasserstein distance between two diagrams.
 *
 * Purpose:
 *   Measure structural change in market topology across days.
 *
 * @param current_diagram Current day's persistence diagram.
 * @param previous_diagram Previous day's persistence diagram.
 * @return Scalar Wasserstein distance.
 *
 * Paper reference: Section 4
 */
double compute_wasserstein_distance(
    const PersistenceDiagram& current_diagram,
    const PersistenceDiagram& previous_diagram
);
