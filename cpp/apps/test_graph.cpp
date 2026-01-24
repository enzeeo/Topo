/**
 * @file test_graph.cpp
 * @brief Test graph construction, Laplacian, and GTV.
 *
 * Usage:
 *   ./test_graph <parquet_file_path>
 */

 #include <iostream>
 #include <cmath>
 
 #include "types.hpp"
 #include "io_parquet.hpp"
 #include "returns.hpp"
 #include "correlation.hpp"
 #include "graph.hpp"
 
 int main(int argc, char* argv[]) {
     if (argc < 2) {
         std::cerr << "Usage: " << argv[0]
                   << " <parquet_file_path>" << std::endl;
         return 1;
     }
 
     std::string parquet_path = argv[1];
 
     try {
         uint32_t number_of_rows = 0;
         uint32_t number_of_assets = 0;
 
         Matrix prices = read_close_prices_parquet(
             parquet_path,
             number_of_rows,
             number_of_assets
         );
 
        if (number_of_rows < 2) {
            throw std::runtime_error("Need at least 2 price rows to compute returns");
        }
        const uint32_t rolling_window_length = number_of_rows - 1;

         Returns returns = compute_log_returns(
             prices,
             number_of_assets,
            rolling_window_length
         );
 
         Matrix correlation = compute_correlation(
             returns.window_returns,
             number_of_assets,
            rolling_window_length
         );
 
         Matrix weighted_graph =
             build_weighted_graph(correlation, number_of_assets);
 
         Matrix laplacian =
             compute_laplacian(weighted_graph, number_of_assets);
 
         double gtv = compute_graph_total_variation(
             returns.latest_return,
             laplacian,
             number_of_assets
         );
 
         // Basic checks
         if (std::isnan(gtv) || std::isinf(gtv)) {
             std::cerr << "GTV is NaN or Inf!" << std::endl;
             return 1;
         }
 
         std::cout << "GTV value: " << gtv << std::endl;
         std::cout << "TEST PASSED" << std::endl;
         return 0;
 
     } catch (const std::exception& e) {
         std::cerr << "ERROR: " << e.what() << std::endl;
         return 1;
     }
 }
 