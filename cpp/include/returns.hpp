#pragma once

#include <string>
#include "types.hpp"

/**
 * @brief Compute log returns from closing prices.
 *
 * Purpose:
 *   Convert prices P_{t,i} into log returns r_{t,i}.
 *
 * Formula:
 *   r_{t,i} = log(P_{t,i} / P_{t-1,i})
 *
 * @param closing_prices Dense matrix of closing prices [T x N].
 * @param number_of_assets Number of assets (N).
 * @param window_length Number of days in the window (m).
 * @return Returns struct containing window_returns [m-1 x N] and latest_return [N].
 *
 * Paper reference: Section 1 (definition of returns)
 */
Returns compute_log_returns(
    const Matrix& closing_prices,
    uint32_t number_of_assets,
    uint32_t window_length
);

/**
 * @brief Save rolling-window returns to a binary file.
 *
 * Purpose:
 *   Persist intermediate results for debugging, benchmarking,
 *   or recomputation without parquet I/O.
 *
 * Binary layout:
 *   - uint32: number of assets (N)
 *   - uint32: window length (m)
 *   - double[m * N]: return matrix (row-major)
 *
 * @param returns Return matrix to save.
 * @param number_of_assets Number of assets (N).
 * @param window_length Window length (m).
 * @param output_path Path to write the binary file.
 */
void save_returns_bin(
    const Matrix& returns,
    uint32_t number_of_assets,
    uint32_t window_length,
    const std::string& output_path
);
