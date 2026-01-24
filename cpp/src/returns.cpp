#include "returns.hpp"

#include <cmath>
#include <fstream>
#include <stdexcept>

Returns compute_log_returns(
    const Matrix& closing_prices,
    uint32_t number_of_assets,
    uint32_t window_length
) {
    // Validate inputs
    size_t expected_size =
        static_cast<size_t>(window_length + 1) * static_cast<size_t>(number_of_assets);
    if (closing_prices.size() != expected_size) {
        throw std::runtime_error("closing_prices size mismatch: expected " + 
                                 std::to_string(expected_size) + 
                                 ", got " + std::to_string(closing_prices.size()));
    }

    if (window_length < 1) {
        throw std::runtime_error("window_length must be at least 1 to compute returns");
    }

    // window_length is the number of return rows.
    // closing_prices provides (window_length + 1) price rows.
    uint32_t number_of_return_days = window_length;

    // Allocate output matrix: [number_of_return_days x number_of_assets]
    Matrix window_returns(
        static_cast<size_t>(number_of_return_days) * static_cast<size_t>(number_of_assets)
    );

    // Compute log returns: r_{t,i} = log(P_{t,i} / P_{t-1,i})
    for (uint32_t day_index = 0; day_index < number_of_return_days; ++day_index) {
        for (uint32_t asset_index = 0; asset_index < number_of_assets; ++asset_index) {
            // Previous day price: P_{t-1,i}
            size_t previous_price_index = day_index * number_of_assets + asset_index;
            double previous_price = closing_prices[previous_price_index];

            // Current day price: P_{t,i}
            size_t current_price_index = (day_index + 1) * number_of_assets + asset_index;
            double current_price = closing_prices[current_price_index];

            // Validate prices are positive
            if (previous_price <= 0.0 || current_price <= 0.0) {
                throw std::runtime_error("Invalid price: prices must be positive for log returns");
            }

            // Log return: log(P_t / P_{t-1})
            double log_return = std::log(current_price / previous_price);

            // Store in row-major order
            size_t return_index = day_index * number_of_assets + asset_index;
            window_returns[return_index] = log_return;
        }
    }

    // Extract latest return vector (last row of window_returns)
    Vector latest_return(number_of_assets);
    size_t last_row_start = (number_of_return_days - 1) * number_of_assets;
    for (uint32_t asset_index = 0; asset_index < number_of_assets; ++asset_index) {
        latest_return[asset_index] = window_returns[last_row_start + asset_index];
    }

    // Build and return the result
    Returns result;
    result.window_returns = std::move(window_returns);
    result.latest_return = std::move(latest_return);

    return result;
}

void save_returns_bin(
    const Matrix& returns,
    uint32_t number_of_assets,
    uint32_t window_length,
    const std::string& output_path
) {
    std::ofstream output_file(output_path, std::ios::binary);
    if (!output_file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + output_path);
    }

    // Write header: number_of_assets (uint32)
    output_file.write(reinterpret_cast<const char*>(&number_of_assets), sizeof(uint32_t));

    // Write header: window_length (uint32)
    output_file.write(reinterpret_cast<const char*>(&window_length), sizeof(uint32_t));

    // Write data: return matrix (double array)
    size_t data_size = returns.size() * sizeof(double);
    output_file.write(reinterpret_cast<const char*>(returns.data()), data_size);

    if (!output_file.good()) {
        throw std::runtime_error("Error writing to file: " + output_path);
    }

    output_file.close();
}
