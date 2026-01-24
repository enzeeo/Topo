#pragma once

#include <string>
#include "types.hpp"

/**
 * @brief Compute final market strain index.
 *
 * Purpose:
 *   Combine volatility, systemic risk, and topology into
 *   a single scalar signal.
 *
 * Formula:
 *   Strain = a * ||r_t||_2 + b * Sys + c * delta_W + d * TP
 *
 * @param l2_return_magnitude L2 norm of latest return vector.
 * @param systemic_ratio Systemic smoothness ratio.
 * @param wasserstein_distance Wasserstein distance to previous diagram.
 * @param total_persistence Total persistence from H1 diagram.
 * @param coefficient_a Weight for L2 return magnitude.
 * @param coefficient_b Weight for systemic ratio.
 * @param coefficient_c Weight for Wasserstein distance.
 * @param coefficient_d Weight for total persistence.
 * @return Scalar strain index.
 *
 * Paper reference: Section 5
 */
double compute_strain_index(
    double l2_return_magnitude,
    double systemic_ratio,
    double wasserstein_distance,
    double total_persistence,
    double coefficient_a,
    double coefficient_b,
    double coefficient_c,
    double coefficient_d
);

/**
 * @brief Write daily strain metrics to a JSON file.
 *
 * Purpose:
 *   Persist the final results of the daily strain computation
 *   in a human-readable and machine-consumable format.
 *
 * JSON schema:
 *   {
 *     "date": "YYYY-MM-DD",
 *     "l2_return_magnitude": <double>,
 *     "graph_total_variation": <double>,
 *     "systemic_ratio": <double>,
 *     "total_persistence": <double>,
 *     "wasserstein_distance": <double>,
 *     "strain_index": <double>
 *   }
 *
 * @param result StrainComponents struct containing all computed metrics.
 * @param output_path Path to write the JSON file.
 *
 * Paper reference: Section 5 (final strain index aggregation)
 */
void write_strain_json(
    const StrainComponents& result,
    const std::string& output_path
);
