#include "io_parquet.hpp"

Matrix read_close_prices_parquet(
    const std::string& parquet_path,
    uint32_t& out_number_of_rows,
    uint32_t& out_number_of_columns
) {
    // TODO: Implement using Arrow library
    throw std::runtime_error("Not implemented");
}
