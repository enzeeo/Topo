#pragma once

#include <string>
#include <vector>

/**
 * @brief Dense matrix type (row-major).
 *
 * Convention:
 *   - Rows correspond to time (or assets, depending on context)
 *   - Columns correspond to assets
 *
 * Used for:
 *   - Price matrices
 *   - Return matrices
 *   - Correlation / distance matrices
 */
using Matrix = std::vector<double>;

/**
 * @brief Dense vector type.
 *
 * Used for:
 *   - Single-day return vector r_t
 *   - Smoothed return vector s_t
 */
using Vector = std::vector<double>;

/**
 * @brief Persistence pair storing birth and death times.
 *
 * Each pair (birth, death) represents the birth and death time
 * of a topological feature (H1 loop).
 */
struct PersistencePair {
    double birth;
    double death;
};

/**
 * @brief Persistence diagram storing birth-death pairs.
 */
using PersistenceDiagram = std::vector<PersistencePair>;

/**
 * @brief Container for log returns computation result.
 *
 * Attributes:
 *   window_returns: Rolling window return matrix [m x N]
 *   latest_return: Latest return vector [N]
 */
struct Returns {
    Matrix window_returns;
    Vector latest_return;
};

/**
 * @brief Container for final daily strain metrics.
 *
 * This struct represents the complete output of one daily run,
 * suitable for serialization to JSON.
 */
struct StrainComponents {
    std::string date;              ///< Latest date represented by the parquet input
    double l2_return_magnitude;    ///< ||r_t||_2 - market-wide volatility magnitude
    double graph_total_variation;  ///< r_t^T L r_t
    double systemic_ratio;         ///< ||s_t|| / ||r_t||
    double total_persistence;      ///< Sum of lifetimes from H1 diagram
    double wasserstein_distance;   ///< Distance to previous diagram
    double strain_index;           ///< Final combined strain index
    double normalized_strain_index;///< (strain_index - mean) / std_pop
};
