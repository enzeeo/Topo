/**
 * @file tune_eta.cpp
 * @brief Calibrate a single global diffusion eta using historical smoothness target.
 *
 * Target (paper):
 *   Smoothness_eta(t) = || exp(-eta * L(t)) r_t ||_2 / || r_t ||_2
 * Choose eta so average Smoothness_eta over T samples is ~ target (default 0.7).
 *
 * Usage:
 *   ./tune_eta --inputs-list <paths.txt>
 *             [--target-lower 0.6] [--target-upper 0.8]
 *             [--tol 0.01] [--max-files 60]
 *             [--eta-lower-start 0.002] [--eta-upper-start 0.006] [--eta-upper-max 64.0]
 *
 * inputs-list format:
 *   One parquet path per line.
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

#include "types.hpp"
#include "io_parquet.hpp"
#include "returns.hpp"
#include "correlation.hpp"
#include "graph.hpp"

struct SmoothnessCache {
    std::string parquet_path;
    std::vector<double> eigenvalues;
    std::vector<double> projected_return_squared;
    double latest_return_norm;
};

struct InputItem {
    /**
     * @brief Path to a single compute-input parquet file.
     *
     * This is typically one of:
     *   data/compute_inputs/prices_window_YYYY-MM-DD.parquet
     *
     * The file is expected to contain a rolling window of close prices used to
     * compute returns, correlation, Laplacian, and smoothness for eta tuning.
     */
    std::string parquet_path;
};

/**
 * @brief True if value is not NaN/Inf.
 */
static bool is_finite(double value) {
    return !(std::isnan(value) || std::isinf(value));
}

/**
 * @brief Compute Euclidean (L2) norm of a vector.
 */
static double l2_norm(const Vector& values) {
    double sum_squares = 0.0;
    for (double v : values) {
        sum_squares += v * v;
    }
    return std::sqrt(sum_squares);
}

/**
 * @brief Read a required CLI argument value.
 *
 * Looks for: <name> <value>
 *
 * @throws std::runtime_error if missing.
 */
static std::string required_arg_value(int argc, char* argv[], const std::string& name) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (name == argv[i]) {
            return std::string(argv[i + 1]);
        }
    }
    throw std::runtime_error("Missing required arg: " + name);
}

/**
 * @brief Read an optional CLI argument value.
 *
 * Looks for: <name> <value>
 *
 * @return default_value if missing.
 */
static std::string optional_arg_value(
    int argc,
    char* argv[],
    const std::string& name,
    const std::string& default_value
) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (name == argv[i]) {
            return std::string(argv[i + 1]);
        }
    }
    return default_value;
}

/**
 * @brief Read an optional integer CLI argument.
 *
 * Looks for: <name> <int>
 *
 * @return default_value if missing.
 */
static int optional_int_value(int argc, char* argv[], const std::string& name, int default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (name == argv[i]) {
            return std::stoi(std::string(argv[i + 1]));
        }
    }
    return default_value;
}

/**
 * @brief Read an optional double CLI argument.
 *
 * Looks for: <name> <double>
 *
 * @return default_value if missing.
 */
static double optional_double_value(int argc, char* argv[], const std::string& name, double default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (name == argv[i]) {
            return std::stod(std::string(argv[i + 1]));
        }
    }
    return default_value;
}

/**
 * @brief Read an inputs list file into a list of parquet paths.
 *
 * File format:
 *   One parquet path per line.
 *
 * Relative paths are resolved relative to the list file's directory.
 */
static std::vector<InputItem> read_inputs_list(const std::string& list_path) {
    std::ifstream input_file(list_path);
    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open inputs list: " + list_path);
    }

    const std::filesystem::path list_parent_directory =
        std::filesystem::path(list_path).parent_path();

    std::vector<InputItem> items;
    std::string line;
    while (std::getline(input_file, line)) {
        if (line.empty()) {
            continue;
        }
        const std::filesystem::path candidate_path(line);
        const std::filesystem::path resolved_path =
            candidate_path.is_absolute()
                ? candidate_path
                : (list_parent_directory / candidate_path);

        items.push_back(InputItem{resolved_path.string()});
    }

    if (items.empty()) {
        throw std::runtime_error("inputs list is empty: " + list_path);
    }

    return items;
}

/**
 * @brief Select up to max_files inputs, evenly spaced across the full list.
 *
 * This reduces runtime while keeping coverage across the entire time span.
 */
static std::vector<InputItem> select_evenly_spaced(
    const std::vector<InputItem>& all,
    int max_files
) {
    if (max_files <= 0) {
        throw std::runtime_error("max_files must be > 0");
    }

    if (static_cast<int>(all.size()) <= max_files) {
        return all;
    }

    std::vector<InputItem> selected;
    selected.reserve(static_cast<size_t>(max_files));

    const int n = static_cast<int>(all.size());
    for (int k = 0; k < max_files; ++k) {
        const double position = (max_files == 1) ? 0.0 : (static_cast<double>(k) / (max_files - 1));
        const int index = static_cast<int>(std::round(position * (n - 1)));
        selected.push_back(all[static_cast<size_t>(index)]);
    }

    return selected;
}

/**
 * @brief Compute average smoothness across a set of input files for a fixed eta.
 *
 * Smoothness_eta(t) = ||s_t||_2 / ||r_t||_2
 * where s_t = exp(-eta * L(t)) r_t
 *
 * Invalid inputs (e.g., missing file, bad values) are skipped with a message.
 */
static double compute_average_smoothness(
    const std::vector<InputItem>& inputs,
    double diffusion_eta
) {
    if (!is_finite(diffusion_eta) || diffusion_eta < 0.0) {
        throw std::runtime_error("diffusion_eta must be finite and >= 0");
    }

    (void)inputs;
    (void)diffusion_eta;
    throw std::runtime_error("compute_average_smoothness(InputItem, eta) removed: use cached version");
}

static SmoothnessCache build_smoothness_cache_or_throw(const InputItem& item) {
    uint32_t number_of_price_rows = 0;
    uint32_t number_of_assets = 0;

    Matrix closing_prices = read_close_prices_parquet(
        item.parquet_path,
        number_of_price_rows,
        number_of_assets
    );

    if (number_of_price_rows < 2) {
        throw std::runtime_error("Need at least 2 price rows");
    }

    const uint32_t rolling_window_length = number_of_price_rows - 1;
    Returns returns = compute_log_returns(closing_prices, number_of_assets, rolling_window_length);

    Matrix correlation = compute_correlation(returns.window_returns, number_of_assets, rolling_window_length);
    Matrix weighted_adjacency = build_weighted_graph(correlation, number_of_assets);
    Matrix laplacian = compute_laplacian(weighted_adjacency, number_of_assets);

    const double latest_norm = l2_norm(returns.latest_return);

    Eigen::MatrixXd laplacian_matrix =
        Eigen::MatrixXd::Zero(static_cast<int>(number_of_assets), static_cast<int>(number_of_assets));
    for (uint32_t row_index = 0; row_index < number_of_assets; ++row_index) {
        for (uint32_t column_index = 0; column_index < number_of_assets; ++column_index) {
            const size_t flat_index =
                static_cast<size_t>(row_index) * static_cast<size_t>(number_of_assets) +
                static_cast<size_t>(column_index);
            laplacian_matrix(static_cast<int>(row_index), static_cast<int>(column_index)) =
                laplacian[flat_index];
        }
    }

    Eigen::VectorXd latest_return_vector = Eigen::VectorXd::Zero(static_cast<int>(number_of_assets));
    for (uint32_t asset_index = 0; asset_index < number_of_assets; ++asset_index) {
        latest_return_vector(static_cast<int>(asset_index)) =
            returns.latest_return[static_cast<size_t>(asset_index)];
    }

    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigen_solver(laplacian_matrix);
    if (eigen_solver.info() != Eigen::Success) {
        throw std::runtime_error("SelfAdjointEigenSolver failed for Laplacian matrix");
    }

    const Eigen::VectorXd eigenvalues = eigen_solver.eigenvalues();
    const Eigen::MatrixXd eigenvectors = eigen_solver.eigenvectors();

    const Eigen::VectorXd projected = eigenvectors.transpose() * latest_return_vector;

    SmoothnessCache cache;
    cache.parquet_path = item.parquet_path;
    cache.latest_return_norm = latest_norm;
    cache.eigenvalues.resize(static_cast<size_t>(number_of_assets));
    cache.projected_return_squared.resize(static_cast<size_t>(number_of_assets));

    for (uint32_t i = 0; i < number_of_assets; ++i) {
        const double lambda_value = eigenvalues(static_cast<int>(i));
        const double projected_value = projected(static_cast<int>(i));
        if (!is_finite(lambda_value) || !is_finite(projected_value)) {
            throw std::runtime_error("Non-finite eigen decomposition results");
        }
        cache.eigenvalues[static_cast<size_t>(i)] = lambda_value;
        cache.projected_return_squared[static_cast<size_t>(i)] = projected_value * projected_value;
    }

    return cache;
}

static std::vector<SmoothnessCache> build_all_smoothness_caches(
    const std::vector<InputItem>& inputs
) {
    std::vector<std::optional<SmoothnessCache>> cache_slots(inputs.size());
    std::vector<std::string> error_messages(inputs.size());

#ifdef _OPENMP
#include <omp.h>
#pragma omp parallel for
#endif
    for (int i = 0; i < static_cast<int>(inputs.size()); ++i) {
        try {
            cache_slots[static_cast<size_t>(i)] = build_smoothness_cache_or_throw(inputs[static_cast<size_t>(i)]);
        } catch (const std::exception& e) {
            error_messages[static_cast<size_t>(i)] = e.what();
        }
    }

    std::vector<SmoothnessCache> caches;
    caches.reserve(inputs.size());

    int skipped_count = 0;
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (cache_slots[i].has_value()) {
            caches.push_back(std::move(cache_slots[i].value()));
            continue;
        }
        skipped_count += 1;
        if (!error_messages[i].empty()) {
            std::cerr << "SKIP " << inputs[i].parquet_path << " : " << error_messages[i] << "\n";
        } else {
            std::cerr << "SKIP " << inputs[i].parquet_path << " : unknown error\n";
        }
    }

    if (caches.empty()) {
        throw std::runtime_error("All inputs were skipped (no valid files to tune eta)");
    }

    if (skipped_count > 0) {
        std::cerr << "Skipped " << skipped_count << " file(s), used " << caches.size() << " file(s)\n";
    }

    return caches;
}

static double compute_average_smoothness_cached(
    const std::vector<SmoothnessCache>& caches,
    double diffusion_eta
) {
    if (!is_finite(diffusion_eta) || diffusion_eta < 0.0) {
        throw std::runtime_error("diffusion_eta must be finite and >= 0");
    }

    std::vector<double> smoothness_values(caches.size(), 0.0);
    std::vector<int> ok_flags(caches.size(), 1);

#ifdef _OPENMP
#include <omp.h>
#pragma omp parallel for
#endif
    for (int i = 0; i < static_cast<int>(caches.size()); ++i) {
        const SmoothnessCache& cache = caches[static_cast<size_t>(i)];
        if (cache.latest_return_norm == 0.0) {
            smoothness_values[static_cast<size_t>(i)] = 0.0;
            ok_flags[static_cast<size_t>(i)] = 1;
            continue;
        }

        double smoothed_sum_squares = 0.0;
        for (size_t k = 0; k < cache.eigenvalues.size(); ++k) {
            const double lambda_value = cache.eigenvalues[k];
            const double coefficient = std::exp(-2.0 * diffusion_eta * lambda_value);
            smoothed_sum_squares += coefficient * cache.projected_return_squared[k];
        }

        const double smoothed_norm = std::sqrt(std::max(0.0, smoothed_sum_squares));
        const double smoothness = smoothed_norm / cache.latest_return_norm;
        if (!is_finite(smoothness)) {
            ok_flags[static_cast<size_t>(i)] = 0;
            smoothness_values[static_cast<size_t>(i)] = std::numeric_limits<double>::quiet_NaN();
            continue;
        }
        smoothness_values[static_cast<size_t>(i)] = smoothness;
        ok_flags[static_cast<size_t>(i)] = 1;
    }

    double sum_smoothness = 0.0;
    for (size_t i = 0; i < smoothness_values.size(); ++i) {
        if (ok_flags[i] == 0) {
            throw std::runtime_error("Smoothness computed as NaN/Inf for cached input");
        }
        sum_smoothness += smoothness_values[i];
    }

    return sum_smoothness / static_cast<double>(smoothness_values.size());
}

/**
 * @brief Find eta such that average smoothness is within a target range.
 *
 * Assumes average smoothness is monotone non-increasing in eta.
 *
 * Strategy:
 * - If smoothness(eta=0) is already within range -> return 0.
 * - Otherwise, find the smallest eta such that smoothness(eta) <= target_upper.
 * - Verify the achieved smoothness is still >= target_lower.
 */
static double calibrate_eta_for_smoothness_range(
    const std::vector<SmoothnessCache>& caches,
    double target_lower,
    double target_upper,
    double tolerance,
    double eta_lower_start,
    double eta_upper_start,
    double eta_upper_max,
    int max_iterations
) {
    if (!is_finite(target_lower) || !is_finite(target_upper)) {
        throw std::runtime_error("target range bounds must be finite");
    }
    if (target_lower < 0.0 || target_upper > 1.0 || target_lower > target_upper) {
        throw std::runtime_error("target range must satisfy 0 <= lower <= upper <= 1");
    }
    if (!is_finite(tolerance) || tolerance <= 0.0) {
        throw std::runtime_error("tol must be > 0");
    }
    if (!is_finite(eta_lower_start) || eta_lower_start < 0.0) {
        throw std::runtime_error("eta_lower_start must be >= 0");
    }
    if (!is_finite(eta_upper_start) || eta_upper_start <= 0.0) {
        throw std::runtime_error("eta_upper_start must be > 0");
    }
    if (!is_finite(eta_upper_max) || eta_upper_max <= eta_upper_start) {
        throw std::runtime_error("eta_upper_max must be > eta_upper_start");
    }
    if (max_iterations <= 0) {
        throw std::runtime_error("max_iterations must be > 0");
    }

    const double smoothness_at_zero = compute_average_smoothness_cached(caches, 0.0);
    if (smoothness_at_zero >= target_lower && smoothness_at_zero <= target_upper) {
        return 0.0;
    }
    if (smoothness_at_zero < target_lower) {
        // Even without smoothing, we are already below the lower bound.
        return 0.0;
    }

    double lower_eta = eta_lower_start;
    double upper_eta = eta_upper_start;

    if (upper_eta <= lower_eta) {
        throw std::runtime_error("eta_upper_start must be > eta_lower_start");
    }

    // If lower_eta already satisfies the upper bound, search below it for the smallest eta.
    const double smoothness_at_lower = compute_average_smoothness_cached(caches, lower_eta);
    if (smoothness_at_lower <= target_upper) {
        upper_eta = lower_eta;
        lower_eta = 0.0;
    }

    while (upper_eta < eta_upper_max) {
        const double smoothness_at_upper = compute_average_smoothness_cached(caches, upper_eta);
        if (smoothness_at_upper <= target_upper) {
            break;
        }
        upper_eta *= 2.0;
    }
    if (upper_eta > eta_upper_max) {
        upper_eta = eta_upper_max;
    }

    for (int iteration = 0; iteration < max_iterations; ++iteration) {
        const double mid_eta = 0.5 * (lower_eta + upper_eta);
        const double mid_smoothness = compute_average_smoothness_cached(caches, mid_eta);

        // We want the smallest eta such that smoothness <= target_upper.
        if (mid_smoothness > target_upper) {
            lower_eta = mid_eta;
        } else {
            upper_eta = mid_eta;
        }
    }

    const double eta = upper_eta;
    const double achieved = compute_average_smoothness_cached(caches, eta);
    if (achieved + tolerance < target_lower) {
        throw std::runtime_error("No eta found in target range within tolerance");
    }
    return eta;
}

/**
 * @brief Entry point for eta tuning.
 *
 * Prints:
 *   eta=<value>
 *   avg_smoothness=<value>
 *   samples_used=<count>
 */
int main(int argc, char* argv[]) {
    const std::string inputs_list_path = required_arg_value(argc, argv, "--inputs-list");
    const double target_lower = optional_double_value(argc, argv, "--target-lower", 0.6);
    const double target_upper = optional_double_value(argc, argv, "--target-upper", 0.7);
    const double single_target = optional_double_value(
        argc,
        argv,
        "--target",
        std::numeric_limits<double>::quiet_NaN()
    );
    const double tol = optional_double_value(argc, argv, "--tol", 0.01);
    const int max_files = optional_int_value(argc, argv, "--max-files", 60);
    const double eta_lower_start = optional_double_value(argc, argv, "--eta-lower-start", 0.001);
    const double eta_upper_start = optional_double_value(argc, argv, "--eta-upper-start", 0.007);
    const double eta_upper_max = optional_double_value(argc, argv, "--eta-upper-max", 64.0);
    const int max_iterations = optional_int_value(argc, argv, "--max-iterations", 20);

    const std::vector<InputItem> all_inputs = read_inputs_list(inputs_list_path);
    const std::vector<InputItem> selected_inputs = select_evenly_spaced(all_inputs, max_files);

    const std::vector<SmoothnessCache> caches = build_all_smoothness_caches(selected_inputs);

    double final_target_lower = target_lower;
    double final_target_upper = target_upper;
    if (is_finite(single_target)) {
        final_target_lower = single_target;
        final_target_upper = single_target;
    }

    const double eta = calibrate_eta_for_smoothness_range(
        caches,
        final_target_lower,
        final_target_upper,
        tol,
        eta_lower_start,
        eta_upper_start,
        eta_upper_max,
        max_iterations
    );

    const double achieved = compute_average_smoothness_cached(caches, eta);

    std::cout << "eta=" << eta << "\n";
    std::cout << "avg_smoothness=" << achieved << "\n";
    std::cout << "samples_used=" << caches.size() << "\n";
    return 0;
}

