/**
 * @file test_returns.cpp
 * @brief Test program for returns computation.
 *
 * Usage:
 *   ./test_returns <path_to_parquet_file>
 *
 * Expected output:
 *   - Log returns computed from prices
 *   - Summary statistics of returns
 *   - Binary file saved successfully
 */

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <numeric>

#include "types.hpp"
#include "io_parquet.hpp"
#include "returns.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <parquet_file_path>" << std::endl;
        return 1;
    }

    std::string parquet_path = argv[1];
    std::cout << "Testing returns computation" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    try {
        // Step 1: Load prices
        std::cout << "Step 1: Loading prices from parquet..." << std::endl;
        uint32_t number_of_rows = 0;
        uint32_t number_of_columns = 0;

        Matrix closing_prices = read_close_prices_parquet(
            parquet_path,
            number_of_rows,
            number_of_columns
        );

        std::cout << "  Loaded " << number_of_rows << " days x " 
                  << number_of_columns << " assets" << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Step 2: Compute log returns
        std::cout << "Step 2: Computing log returns..." << std::endl;
        if (number_of_rows < 2) {
            throw std::runtime_error("Need at least 2 price rows to compute returns");
        }
        uint32_t rolling_window_length = number_of_rows - 1;
        Returns returns = compute_log_returns(
            closing_prices,
            number_of_columns,  // number_of_assets
            rolling_window_length
        );

        uint32_t return_days = rolling_window_length;
        std::cout << "  Window returns shape: " << return_days << " days x " 
                  << number_of_columns << " assets" << std::endl;
        std::cout << "  Latest return vector: " << returns.latest_return.size() 
                  << " assets" << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Step 3: Display first 5x5 corner of returns
        std::cout << "Step 3: First 5x5 corner of log returns:" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        
        int display_rows = std::min(5u, return_days);
        int display_cols = std::min(5u, number_of_columns);
        
        for (int row = 0; row < display_rows; ++row) {
            std::cout << "  Day " << row << ": ";
            for (int col = 0; col < display_cols; ++col) {
                size_t index = row * number_of_columns + col;
                std::cout << std::setw(12) << returns.window_returns[index] << " ";
            }
            std::cout << "..." << std::endl;
        }
        std::cout << std::string(60, '-') << std::endl;

        // Step 4: Summary statistics
        std::cout << "Step 4: Summary statistics of all returns:" << std::endl;
        
        double min_return = *std::min_element(
            returns.window_returns.begin(), 
            returns.window_returns.end()
        );
        double max_return = *std::max_element(
            returns.window_returns.begin(), 
            returns.window_returns.end()
        );
        double sum_returns = std::accumulate(
            returns.window_returns.begin(), 
            returns.window_returns.end(), 
            0.0
        );
        double mean_return = sum_returns / returns.window_returns.size();

        // Standard deviation
        double sum_sq_diff = 0.0;
        for (const double& r : returns.window_returns) {
            sum_sq_diff += (r - mean_return) * (r - mean_return);
        }
        double std_return = std::sqrt(sum_sq_diff / returns.window_returns.size());

        std::cout << "  Min return:  " << std::setw(12) << min_return << std::endl;
        std::cout << "  Max return:  " << std::setw(12) << max_return << std::endl;
        std::cout << "  Mean return: " << std::setw(12) << mean_return << std::endl;
        std::cout << "  Std dev:     " << std::setw(12) << std_return << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Step 5: Latest return summary
        std::cout << "Step 5: Latest return vector (r_t):" << std::endl;
        double latest_min = *std::min_element(
            returns.latest_return.begin(), 
            returns.latest_return.end()
        );
        double latest_max = *std::max_element(
            returns.latest_return.begin(), 
            returns.latest_return.end()
        );
        double latest_sum = std::accumulate(
            returns.latest_return.begin(), 
            returns.latest_return.end(), 
            0.0
        );
        double latest_mean = latest_sum / returns.latest_return.size();

        // L2 norm of latest return
        double sum_sq = 0.0;
        for (const double& r : returns.latest_return) {
            sum_sq += r * r;
        }
        double l2_norm = std::sqrt(sum_sq);

        std::cout << "  Min:     " << std::setw(12) << latest_min << std::endl;
        std::cout << "  Max:     " << std::setw(12) << latest_max << std::endl;
        std::cout << "  Mean:    " << std::setw(12) << latest_mean << std::endl;
        std::cout << "  L2 norm: " << std::setw(12) << l2_norm << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Step 6: Save to binary file
        std::cout << "Step 6: Saving returns to binary file..." << std::endl;
        std::string output_path = "test_returns.bin";
        save_returns_bin(
            returns.window_returns,
            number_of_columns,
            rolling_window_length,
            output_path
        );
        std::cout << "  Saved to: " << output_path << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Validation checks
        std::cout << "Validation checks:" << std::endl;
        bool has_nan = false;
        bool has_inf = false;
        for (const double& r : returns.window_returns) {
            if (std::isnan(r)) has_nan = true;
            if (std::isinf(r)) has_inf = true;
        }
        std::cout << "  Contains NaN: " << (has_nan ? "YES (BAD)" : "NO (GOOD)") << std::endl;
        std::cout << "  Contains Inf: " << (has_inf ? "YES (BAD)" : "NO (GOOD)") << std::endl;
        
        // Reasonable range check for daily log returns
        bool reasonable_range = (min_return > -0.5 && max_return < 0.5);
        std::cout << "  Returns in [-50%, +50%]: " 
                  << (reasonable_range ? "YES (GOOD)" : "NO (CHECK DATA)") << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        if (has_nan || has_inf) {
            std::cout << "TEST FAILED: Invalid values detected!" << std::endl;
            return 1;
        }

        std::cout << "TEST PASSED: returns computation is working correctly!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
