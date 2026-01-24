#pragma once

#include <string>
#include "types.hpp"

/**
 * @brief Read a parquet file of closing prices into a dense matrix.
 *
 * Purpose:
 *   Load the raw market data P_{t,i} used to compute returns.
 *
 * Input:
 *   Parquet file with shape [T x N]:
 *     - Rows: trading dates (ascending)
 *     - Columns: ticker symbols (alphabetically sorted)
 *
 * Output:
 *   Dense matrix of closing prices (row-major)
 *
 * @param parquet_path Path to the prices_window.parquet file.
 * @param out_number_of_rows Output: number of rows read.
 * @param out_number_of_columns Output: number of columns read.
 * @return Matrix of closing prices in row-major order.
 *
 * Paper reference: Section 1 (input prices for log returns)
 */
Matrix read_close_prices_parquet(
    const std::string& parquet_path,
    uint32_t& out_number_of_rows,
    uint32_t& out_number_of_columns
);
