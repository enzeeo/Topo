/**
 * @file test_topology.cpp
 * @brief Test topology stage (distance transform, H1 persistence, TP, Wasserstein).
 *
 * Usage:
 *   ./test_topology <parquet_file_path> [previous_diagram_bin_path]
 *
 * Notes:
 * - If previous_diagram_bin_path is omitted, Wasserstein is skipped.
 * - This test uses:
 *   - compute_persistence_H1() (ripser embedded)
 *   - compute_wasserstein_distance() (hera)
 */

#include <iostream>
#include <string>
#include <cmath>

#include "types.hpp"
#include "io_parquet.hpp"
#include "returns.hpp"
#include "correlation.hpp"
#include "topology.hpp"

static bool is_finite(double value) {
    return !(std::isnan(value) || std::isinf(value));
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <parquet_file_path> [previous_diagram_bin_path]" << std::endl;
        return 1;
    }

    const std::string parquet_path = argv[1];
    const bool has_previous = (argc >= 3);
    const std::string previous_diagram_path = has_previous ? argv[2] : "";

    try {
        // Load prices
        uint32_t number_of_rows = 0;
        uint32_t number_of_assets = 0;
        Matrix closing_prices = read_close_prices_parquet(parquet_path, number_of_rows, number_of_assets);

        if (number_of_rows < 2) {
            throw std::runtime_error("Need at least 2 price rows to compute returns");
        }
        const uint32_t rolling_window_length = number_of_rows - 1;

        // Returns
        Returns returns = compute_log_returns(closing_prices, number_of_assets, rolling_window_length);

        // Correlation
        Matrix correlation = compute_correlation(returns.window_returns, number_of_assets, rolling_window_length);

        // Distance transform
        Matrix distance_matrix = correlation_to_distance(correlation, number_of_assets);

        // Persistence H1
        PersistenceDiagram diagram = compute_persistence_H1(distance_matrix, number_of_assets);
        std::cout << "H1 pairs: " << diagram.size() << std::endl;

        // Total persistence
        double total_persistence = compute_total_persistence(diagram);
        std::cout << "Total persistence: " << total_persistence << std::endl;

        if (!is_finite(total_persistence) || total_persistence < 0.0) {
            std::cerr << "Invalid total persistence" << std::endl;
            return 1;
        }

        // Save current diagram
        const std::string current_diagram_path = "test_diagram.bin";
        save_diagram_bin(diagram, current_diagram_path);
        std::cout << "Wrote diagram: " << current_diagram_path << std::endl;

        // Wasserstein distance (optional)
        if (has_previous) {
            PersistenceDiagram previous_diagram = load_diagram_bin(previous_diagram_path);
            double wasserstein = compute_wasserstein_distance(diagram, previous_diagram);
            std::cout << "Wasserstein distance: " << wasserstein << std::endl;

            if (!is_finite(wasserstein) || wasserstein < 0.0) {
                std::cerr << "Invalid Wasserstein distance" << std::endl;
                return 1;
            }
        }

        std::cout << "TEST PASSED" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}

