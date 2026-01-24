/**
 * @file test_diffusion.cpp
 * @brief Test program for diffusion (exp(-eta * L) * r) and systemic ratio.
 *
 * Usage:
 *   ./test_diffusion <path_to_parquet_file> [diffusion_eta]
 */

 #include <iostream>
 #include <iomanip>
 #include <algorithm>
 #include <cmath>
 #include <string>
 
 #include "types.hpp"
 #include "io_parquet.hpp"
 #include "returns.hpp"
 #include "correlation.hpp"
 #include "graph.hpp"
 #include "diffusion.hpp"
 
 static bool is_finite(double value) {
     return !(std::isnan(value) || std::isinf(value));
 }
 
 static double max_abs_difference(const Vector& a, const Vector& b) {
     if (a.size() != b.size()) {
         return std::numeric_limits<double>::infinity();
     }
 
     double max_difference = 0.0;
     for (size_t i = 0; i < a.size(); ++i) {
         const double diff = std::abs(a[i] - b[i]);
         if (diff > max_difference) {
             max_difference = diff;
         }
     }
     return max_difference;
 }
 
 int main(int argc, char* argv[]) {
     if (argc < 2) {
         std::cerr << "Usage: " << argv[0] << " <parquet_file_path> [diffusion_eta]" << std::endl;
         return 1;
     }
 
     const std::string parquet_path = argv[1];
 
     double diffusion_eta = 1.0;
     if (argc >= 3) {
         diffusion_eta = std::stod(argv[2]);
     }
 
     std::cout << "Testing diffusion + systemic ratio" << std::endl;
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
        if (number_of_rows < 2) {
            throw std::runtime_error("Need at least 2 price rows to compute returns");
        }
        const uint32_t rolling_window_length = number_of_rows - 1;
         Returns returns = compute_log_returns(
             closing_prices,
             number_of_assets,
            rolling_window_length
         );
 
        const uint32_t return_days = rolling_window_length;
         std::cout << "  Returns shape: " << return_days << " x "
                   << number_of_assets << std::endl;
         std::cout << std::string(60, '-') << std::endl;
 
         // Step 3: Correlation
         std::cout << "Step 3: Computing correlation..." << std::endl;
         Matrix correlation = compute_correlation(
             returns.window_returns,
             number_of_assets,
            rolling_window_length
         );
 
         // Step 4: Weighted graph and Laplacian
         std::cout << "Step 4: Building graph + Laplacian..." << std::endl;
         Matrix weighted_adjacency = build_weighted_graph(correlation, number_of_assets);
         Matrix laplacian = compute_laplacian(weighted_adjacency, number_of_assets);
 
         // Step 5: Diffuse returns
         std::cout << "Step 5: Diffusing returns..." << std::endl;
         std::cout << "  Using diffusion_eta = " << diffusion_eta << std::endl;
 
         Vector smoothed_return = diffuse_returns(
             laplacian,
             returns.latest_return,
             number_of_assets,
             diffusion_eta
         );
 
         // Step 6: Systemic ratio
         std::cout << "Step 6: Computing systemic ratio..." << std::endl;
         double systemic_ratio = compute_systemic_ratio(smoothed_return, returns.latest_return);
 
         std::cout << std::fixed << std::setprecision(6);
         std::cout << "  Systemic ratio: " << systemic_ratio << std::endl;
         std::cout << std::string(60, '-') << std::endl;
 
         // Step 7: Validations
         std::cout << "Step 7: Validation checks..." << std::endl;
 
         bool smoothed_finite = true;
         for (const double& value : smoothed_return) {
             if (!is_finite(value)) {
                 smoothed_finite = false;
                 break;
             }
         }
         std::cout << "  Smoothed return finite (no NaN/Inf): "
                   << (smoothed_finite ? "YES (GOOD)" : "NO (BAD)") << std::endl;
 
         bool ratio_finite = is_finite(systemic_ratio);
         std::cout << "  Systemic ratio finite: "
                   << (ratio_finite ? "YES (GOOD)" : "NO (BAD)") << std::endl;
 
         // If diffusion_eta = 0, exp(0) = I, so smoothed_return should match latest_return.
         Vector smoothed_eta_zero = diffuse_returns(
             laplacian,
             returns.latest_return,
             number_of_assets,
             0.0
         );
 
         double max_diff_eta_zero = max_abs_difference(smoothed_eta_zero, returns.latest_return);
         bool eta_zero_identity_ok = (max_diff_eta_zero <= 1e-10);
 
         std::cout << "  eta=0 behaves like identity (max abs diff <= 1e-10): "
                   << (eta_zero_identity_ok ? "YES (GOOD)" : "NO (BAD)") << std::endl;
 
         // For eta >= 0 and Laplacian PSD, ||s|| <= ||r||. Allow small numeric slack.
         bool ratio_reasonable = true;
         if (diffusion_eta >= 0.0) {
             ratio_reasonable = (systemic_ratio <= 1.0 + 1e-8);
         }
         std::cout << "  If eta>=0, systemic_ratio <= 1 (+eps): "
                   << (ratio_reasonable ? "YES (GOOD)" : "NO (BAD)") << std::endl;
 
         if (!smoothed_finite || !ratio_finite || !eta_zero_identity_ok || !ratio_reasonable) {
             std::cout << "TEST FAILED: Validation issues detected!" << std::endl;
             return 1;
         }
 
         std::cout << "TEST PASSED: diffusion + systemic ratio are working correctly!" << std::endl;
         return 0;
 
     } catch (const std::exception& e) {
         std::cerr << "ERROR: " << e.what() << std::endl;
         return 1;
     }
 }
 