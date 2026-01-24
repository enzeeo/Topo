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

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <vector>

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

static std::string get_env_or_empty(const char* name) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string();
}

static void set_timezone_to_new_york() {
    setenv("TZ", "America/New_York", 1);
    tzset();
}

static std::string format_date_yyyy_mm_dd(const std::tm& time_parts) {
    char buffer[11];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%04d-%02d-%02d",
        time_parts.tm_year + 1900,
        time_parts.tm_mon + 1,
        time_parts.tm_mday
    );
    return std::string(buffer);
}

static std::string today_new_york_date_string() {
    set_timezone_to_new_york();
    const std::time_t now = std::time(nullptr);
    std::tm local_time_parts{};
    localtime_r(&now, &local_time_parts);
    return format_date_yyyy_mm_dd(local_time_parts);
}

static std::string decrement_date_string_by_days(const std::string& date_yyyy_mm_dd, int days_back) {
    int year = 0;
    int month = 0;
    int day = 0;
    if (std::sscanf(date_yyyy_mm_dd.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
        throw std::runtime_error("Invalid date format (expected YYYY-MM-DD): " + date_yyyy_mm_dd);
    }

    set_timezone_to_new_york();

    std::tm time_parts{};
    time_parts.tm_year = year - 1900;
    time_parts.tm_mon = month - 1;
    time_parts.tm_mday = day;
    time_parts.tm_hour = 12;
    time_parts.tm_min = 0;
    time_parts.tm_sec = 0;

    const std::time_t normalized = std::mktime(&time_parts);
    if (normalized == static_cast<std::time_t>(-1)) {
        throw std::runtime_error("Failed to normalize date: " + date_yyyy_mm_dd);
    }

    const std::time_t previous_time = normalized - static_cast<std::time_t>(days_back) * 24 * 60 * 60;
    std::tm previous_parts{};
    localtime_r(&previous_time, &previous_parts);
    return format_date_yyyy_mm_dd(previous_parts);
}

static bool file_exists(const std::string& path) {
    return std::filesystem::exists(std::filesystem::path(path));
}

static std::string join_path(const std::string& a, const std::string& b) {
    return (std::filesystem::path(a) / std::filesystem::path(b)).string();
}

static std::string find_previous_diagram_path(
    const std::string& output_root,
    const std::string& run_date,
    int max_lookback_days
) {
    for (int days_back = 1; days_back <= max_lookback_days; ++days_back) {
        const std::string previous_date = decrement_date_string_by_days(run_date, days_back);
        const std::string previous_dir = join_path(output_root, "date=" + previous_date);
        const std::string previous_diagram_path = join_path(previous_dir, "diagram.bin");
        if (file_exists(previous_diagram_path)) {
            return previous_diagram_path;
        }
    }
    return std::string();
}

static double parse_double_or_default(const std::string& text, double default_value) {
    if (text.empty()) {
        return default_value;
    }
    return std::stod(text);
}

static std::string parse_required_arg(int argc, char* argv[], const std::string& name) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == name) {
            return std::string(argv[i + 1]);
        }
    }
    throw std::runtime_error("Missing required argument: " + name);
}

static std::string parse_optional_arg(int argc, char* argv[], const std::string& name, const std::string& default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == name) {
            return std::string(argv[i + 1]);
        }
    }
    return default_value;
}

static double parse_optional_double_arg(int argc, char* argv[], const std::string& name, double default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == name) {
            return std::stod(std::string(argv[i + 1]));
        }
    }
    return default_value;
}

static double compute_normalized_strain_index(double strain_index, double mean, double std_pop) {
    if (std::isnan(strain_index) || std::isinf(strain_index)) {
        throw std::runtime_error("strain_index must be finite for normalization");
    }
    if (std::isnan(mean) || std::isinf(mean)) {
        throw std::runtime_error("strain_mean must be finite");
    }
    if (std::isnan(std_pop) || std::isinf(std_pop) || std_pop <= 0.0) {
        throw std::runtime_error("strain_std_pop must be finite and > 0");
    }
    return (strain_index - mean) / std_pop;
}

static double l2_norm(const Vector& values) {
    double sum_squares = 0.0;
    for (double v : values) {
        sum_squares += v * v;
    }
    return std::sqrt(sum_squares);
}

int main(int argc, char* argv[]) {
    const std::string parquet_input_path = parse_required_arg(argc, argv, "--input");
    const std::string output_root = parse_optional_arg(argc, argv, "--output", "out/daily");

    std::string run_date = parse_optional_arg(argc, argv, "--date", "");
    if (run_date.empty()) {
        const std::string run_date_env = get_env_or_empty("RUN_DATE");
        run_date = run_date_env.empty() ? today_new_york_date_string() : run_date_env;
    }

    const double diffusion_eta = parse_optional_double_arg(argc, argv, "--eta", 1.0);
    const double coefficient_a = parse_optional_double_arg(argc, argv, "--a", 1.0);
    const double coefficient_b = parse_optional_double_arg(argc, argv, "--b", 1.0);
    const double coefficient_c = parse_optional_double_arg(argc, argv, "--c", 1.0);
    const double coefficient_d = parse_optional_double_arg(argc, argv, "--d", 1.0);
    const double coefficient_e = parse_optional_double_arg(argc, argv, "--e", 1.0);

    // Normalization parameters (defaults from contracts/params.md for current calibration run)
    const double strain_mean = parse_optional_double_arg(argc, argv, "--strain-mean", 14.998030683897552);
    const double strain_std_pop = parse_optional_double_arg(argc, argv, "--strain-std-pop", 3.134706030763302);

    const std::string output_dir = join_path(output_root, "date=" + run_date);
    std::filesystem::create_directories(std::filesystem::path(output_dir));

    const std::string returns_bin_path = join_path(output_dir, "returns.bin");
    const std::string corr_bin_path = join_path(output_dir, "corr.bin");
    const std::string diagram_bin_path = join_path(output_dir, "diagram.bin");
    const std::string strain_json_path = join_path(output_dir, "strain.json");

    // 1) read_close_prices_parquet
    uint32_t number_of_price_rows = 0;
    uint32_t number_of_assets = 0;
    Matrix closing_prices = read_close_prices_parquet(
        parquet_input_path,
        number_of_price_rows,
        number_of_assets
    );

    if (number_of_price_rows < 2) {
        throw std::runtime_error("prices_window.parquet must contain at least 2 price rows");
    }

    const uint32_t rolling_window_length = number_of_price_rows - 1;
    if (rolling_window_length != 50) {
        std::cerr
            << "WARNING: rolling_window_length expected 50 returns, got "
            << rolling_window_length
            << std::endl;
    }

    // 2) compute_log_returns
    Returns returns = compute_log_returns(closing_prices, number_of_assets, rolling_window_length);

    // 3) save_returns_bin
    save_returns_bin(returns.window_returns, number_of_assets, rolling_window_length, returns_bin_path);

    // 4) compute_correlation
    Matrix correlation = compute_correlation(returns.window_returns, number_of_assets, rolling_window_length);

    // 5) save_correlation_matrix_bin
    save_correlation_bin(correlation, number_of_assets, corr_bin_path);

    // 6) build_weighted_adjacency_matrix
    Matrix weighted_adjacency = build_weighted_graph(correlation, number_of_assets);

    // 7) compute_graph_laplacian
    Matrix laplacian = compute_laplacian(weighted_adjacency, number_of_assets);

    // 8) compute_graph_total_variation
    const double graph_total_variation = compute_graph_total_variation(
        returns.latest_return,
        laplacian,
        number_of_assets
    );

    // 9) diffuse_return_vectors
    Vector smoothed_return = diffuse_returns(laplacian, returns.latest_return, number_of_assets, diffusion_eta);

    // 10) compute_systemic_ratio
    const double systemic_ratio = compute_systemic_ratio(smoothed_return, returns.latest_return);

    // 11) convert_correlation_to_distance
    Matrix distance_matrix = correlation_to_distance(correlation, number_of_assets);

    // 12) compute_persistent_homology_H1
    PersistenceDiagram diagram = compute_persistence_H1(distance_matrix, number_of_assets);

    // 13) save_persistence_diagram_bin
    save_diagram_bin(diagram, diagram_bin_path);

    // 14) compute_total_persistence
    const double total_persistence = compute_total_persistence(diagram);

    // 15) load_previous_persistence_diagram
    const std::string previous_diagram_path = find_previous_diagram_path(output_root, run_date, 7);
    PersistenceDiagram previous_diagram;
    bool has_previous_diagram = false;
    if (!previous_diagram_path.empty()) {
        previous_diagram = load_diagram_bin(previous_diagram_path);
        has_previous_diagram = true;
    }

    // 16) compute_wasserstein_distance
    const double wasserstein_distance = has_previous_diagram
        ? compute_wasserstein_distance(diagram, previous_diagram)
        : 0.0;

    // 17) compute_strain_index
    const double l2_return_magnitude = l2_norm(returns.latest_return);
    const double strain_index = compute_strain_index(
        l2_return_magnitude,
        graph_total_variation,
        systemic_ratio,
        wasserstein_distance,
        total_persistence,
        coefficient_a,
        coefficient_e,
        coefficient_b,
        coefficient_c,
        coefficient_d
    );

    // 18) write_strain_json
    StrainComponents components;
    components.date = run_date;
    components.l2_return_magnitude = l2_return_magnitude;
    components.graph_total_variation = graph_total_variation;
    components.systemic_ratio = systemic_ratio;
    components.total_persistence = total_persistence;
    components.wasserstein_distance = wasserstein_distance;
    components.strain_index = strain_index;
    components.normalized_strain_index = compute_normalized_strain_index(
        strain_index,
        strain_mean,
        strain_std_pop
    );

    write_strain_json(components, strain_json_path);

    return 0;
}
