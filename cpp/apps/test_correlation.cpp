/**
 * @file test_correlation.cpp
 * @brief Test program for correlation matrix computation.
 *
 * Usage:
 *   ./test_correlation <path_to_parquet_file>
 */

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

#include "types.hpp"
#include "io_parquet.hpp"
#include "returns.hpp"
#include "correlation.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <parquet_file_path>" << std::endl;
        return 1;
    }

    std::string parquet_path = argv[1];
    std::cout << "Testing correlation matrix computation" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    try {
        // Step 1: Load prices
        std::cout << "Step 1: Loading prices..." << std::endl;
        uint32_t number_of_rows = 0;
        uint32_t number_of_assets = 0;

        Matrix closing_prices = read_close_prices_parquet(
            parquet_path,
            number_of_rows,
            number_of_assets
        );
        std::cout << "  Loaded " << number_of_rows << " days x " 
                  << number_of_assets << " assets" << std::endl;

        // Step 2: Compute returns
        std::cout << "Step 2: Computing returns..." << std::endl;
        Returns returns = compute_log_returns(
            closing_prices,
            number_of_assets,
            number_of_rows
        );
        uint32_t return_days = number_of_rows - 1;
        std::cout << "  Returns shape: " << return_days << " x " 
                  << number_of_assets << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Step 3: Compute correlation matrix
        std::cout << "Step 3: Computing correlation matrix..." << std::endl;
        Matrix correlation = compute_correlation(
            returns.window_returns,
            number_of_assets,
            return_days
        );
        std::cout << "  Correlation matrix shape: " << number_of_assets 
                  << " x " << number_of_assets << std::endl;
        std::cout << "  Total elements: " << correlation.size() << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Step 4: Display first 5x5 corner
        std::cout << "Step 4: First 5x5 corner of correlation matrix:" << std::endl;
        std::cout << std::fixed << std::setprecision(4);
        
        int display_size = std::min(5u, number_of_assets);
        
        for (int i = 0; i < display_size; ++i) {
            std::cout << "  ";
            for (int j = 0; j < display_size; ++j) {
                size_t index = i * number_of_assets + j;
                std::cout << std::setw(8) << correlation[index] << " ";
            }
            std::cout << "..." << std::endl;
        }
        std::cout << std::string(60, '-') << std::endl;

        // Step 5: Validation checks
        std::cout << "Step 5: Validation checks:" << std::endl;
        
        // Check diagonal is 1.0
        bool diagonal_ok = true;
        for (uint32_t i = 0; i < number_of_assets; ++i) {
            double diag_val = correlation[i * number_of_assets + i];
            if (std::abs(diag_val - 1.0) > 1e-6) {
                diagonal_ok = false;
                break;
            }
        }
        std::cout << "  Diagonal = 1.0: " 
                  << (diagonal_ok ? "YES (GOOD)" : "NO (BAD)") << std::endl;

        // Check symmetry
        bool symmetric = true;
        for (uint32_t i = 0; i < number_of_assets && symmetric; ++i) {
            for (uint32_t j = i + 1; j < number_of_assets; ++j) {
                double val_ij = correlation[i * number_of_assets + j];
                double val_ji = correlation[j * number_of_assets + i];
                if (std::abs(val_ij - val_ji) > 1e-10) {
                    symmetric = false;
                    break;
                }
            }
        }
        std::cout << "  Matrix symmetric: " 
                  << (symmetric ? "YES (GOOD)" : "NO (BAD)") << std::endl;

        // Check range [-1, 1]
        double min_corr = *std::min_element(correlation.begin(), correlation.end());
        double max_corr = *std::max_element(correlation.begin(), correlation.end());
        bool range_ok = (min_corr >= -1.0 - 1e-6) && (max_corr <= 1.0 + 1e-6);
        std::cout << "  Range [-1, 1]: " 
                  << (range_ok ? "YES (GOOD)" : "NO (BAD)") << std::endl;
        std::cout << "    Min correlation: " << min_corr << std::endl;
        std::cout << "    Max correlation: " << max_corr << std::endl;

        // Check for NaN/Inf
        bool has_nan = false;
        bool has_inf = false;
        for (const double& c : correlation) {
            if (std::isnan(c)) has_nan = true;
            if (std::isinf(c)) has_inf = true;
        }
        std::cout << "  Contains NaN: " 
                  << (has_nan ? "YES (BAD)" : "NO (GOOD)") << std::endl;
        std::cout << "  Contains Inf: " 
                  << (has_inf ? "YES (BAD)" : "NO (GOOD)") << std::endl;

        // Summary statistics
        double sum = 0.0;
        int count_positive = 0;
        int count_negative = 0;
        for (uint32_t i = 0; i < number_of_assets; ++i) {
            for (uint32_t j = i + 1; j < number_of_assets; ++j) {
                double val = correlation[i * number_of_assets + j];
                sum += val;
                if (val > 0) count_positive++;
                else if (val < 0) count_negative++;
            }
        }
        int total_pairs = (number_of_assets * (number_of_assets - 1)) / 2;
        double mean_corr = sum / total_pairs;
        
        std::cout << "  Mean off-diagonal correlation: " << mean_corr << std::endl;
        std::cout << "  Positive correlations: " << count_positive 
                  << " (" << (100.0 * count_positive / total_pairs) << "%)" << std::endl;
        std::cout << "  Negative correlations: " << count_negative 
                  << " (" << (100.0 * count_negative / total_pairs) << "%)" << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Step 6: Save to binary
        std::cout << "Step 6: Saving correlation matrix..." << std::endl;
        std::string output_path = "test_correlation.bin";
        save_correlation_bin(correlation, number_of_assets, output_path);
        std::cout << "  Saved to: " << output_path << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        if (!diagonal_ok || !symmetric || !range_ok || has_nan || has_inf) {
            std::cout << "TEST FAILED: Validation issues detected!" << std::endl;
            return 1;
        }

        std::cout << "TEST PASSED: correlation computation is working correctly!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
