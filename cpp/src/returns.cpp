#include "returns.hpp"

Returns compute_log_returns(
    const Matrix& closing_prices,
    uint32_t number_of_assets,
    uint32_t window_length
) {
    // TODO: Implement log return computation
    throw std::runtime_error("Not implemented");
}

void save_returns_bin(
    const Matrix& returns,
    uint32_t number_of_assets,
    uint32_t window_length,
    const std::string& output_path
) {
    // TODO: Implement binary serialization
    throw std::runtime_error("Not implemented");
}
