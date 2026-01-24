/**
 * @file daily_run.cpp
 * @brief Orchestrator for daily strain index computation.
 *
 * This file executes the computation pipeline in the required order.
 * DO NOT change the execution order.
 *
 * Required execution order:
 *   1. read_close_prices_parquet
 *   2. compute_log_returns
 *   3. save_returns_bin
 *   4. compute_correlation
 *   5. save_correlation_bin
 *   6. build_weighted_graph
 *   7. compute_laplacian
 *   8. compute_graph_total_variation
 *   9. diffuse_returns
 *   10. compute_systemic_ratio
 *   11. correlation_to_distance
 *   12. compute_persistence_H1
 *   13. save_diagram_bin
 *   14. compute_total_persistence
 *   15. load_diagram_bin (previous day)
 *   16. compute_wasserstein_distance
 *   17. compute_strain_index
 *   18. write_strain_json
 */

#include <iostream>
#include <string>

#include "types.hpp"
#include "io_parquet.hpp"
#include "returns.hpp"
#include "correlation.hpp"
#include "graph.hpp"
#include "diffusion.hpp"
#include "topology.hpp"
#include "strain.hpp"

int main(int argc, char* argv[]) {
    // TODO: Parse command line arguments for:
    //   - input parquet path
    //   - output directory
    //   - previous diagram path (for Wasserstein distance)
    //   - hyperparameters (eta, a, b, c, d)

    // TODO: Execute pipeline in required order

    std::cout << "Daily strain computation not yet implemented." << std::endl;
    return 1;
}
