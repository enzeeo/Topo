#include "io_parquet.hpp"

#include <arrow/api.h>
#include <arrow/compute/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

#include <iostream>
#include <stdexcept>

Matrix read_close_prices_parquet(
    const std::string& parquet_path,
    uint32_t& out_number_of_rows,
    uint32_t& out_number_of_columns
) {
    // Step 1: Open the parquet file
    auto file_result = arrow::io::ReadableFile::Open(parquet_path);
    if (!file_result.ok()) {
        throw std::runtime_error("Failed to open parquet file: " + parquet_path);
    }
    std::shared_ptr<arrow::io::ReadableFile> input_file = file_result.ValueOrDie();

    // Step 2: Create parquet reader using the builder pattern (newer API)
    parquet::arrow::FileReaderBuilder reader_builder;
    auto builder_status = reader_builder.Open(input_file);
    if (!builder_status.ok()) {
        throw std::runtime_error("Failed to open parquet reader: " + builder_status.ToString());
    }

    std::unique_ptr<parquet::arrow::FileReader> parquet_reader;
    auto build_status = reader_builder.Build(&parquet_reader);
    if (!build_status.ok()) {
        throw std::runtime_error("Failed to build parquet reader: " + build_status.ToString());
    }

    // Step 3: Read entire file into Arrow table
    std::shared_ptr<arrow::Table> table;
    auto read_status = parquet_reader->ReadTable(&table);
    if (!read_status.ok()) {
        throw std::runtime_error("Failed to read parquet table: " + read_status.ToString());
    }

    // Step 4: Identify which columns are actual price data (skip date/index columns)
    std::vector<int> price_column_indices;
    for (int col_idx = 0; col_idx < table->num_columns(); ++col_idx) {
        std::string column_name = table->field(col_idx)->name();
        
        // Skip columns that are not price data
        bool is_skip_column = (column_name == "Date") ||
                              (column_name == "date") ||
                              (column_name.find("__index") != std::string::npos) ||
                              (column_name.find("index") == 0) ||
                              (column_name == "");
        
        if (!is_skip_column) {
            price_column_indices.push_back(col_idx);
        }
    }

    // Step 5: Extract dimensions
    int64_t number_of_rows = table->num_rows();
    int number_of_columns = static_cast<int>(price_column_indices.size());

    out_number_of_rows = static_cast<uint32_t>(number_of_rows);
    out_number_of_columns = static_cast<uint32_t>(number_of_columns);

    // Step 6: Allocate output matrix (row-major: rows x columns)
    Matrix closing_prices(number_of_rows * number_of_columns);

    // Step 7: Extract data from each price column (skipping index columns)
    for (int output_col_idx = 0; output_col_idx < number_of_columns; ++output_col_idx) {
        int source_col_idx = price_column_indices[output_col_idx];
        std::shared_ptr<arrow::ChunkedArray> chunked_column = table->column(source_col_idx);
        int64_t row_offset = 0;
        const std::string column_name = table->field(source_col_idx)->name();
        for (int chunk_index = 0; chunk_index < chunked_column->num_chunks(); ++chunk_index) {
            std::shared_ptr<arrow::Array> chunk = chunked_column->chunk(chunk_index);

            std::shared_ptr<arrow::Array> float64_array = chunk;
            if (chunk->type_id() != arrow::Type::DOUBLE) {
                auto cast_result = arrow::compute::Cast(chunk, arrow::float64());
                if (!cast_result.status().ok()) {
                    throw std::runtime_error("Failed to cast parquet column to float64");
                }
                float64_array = cast_result->make_array();
            }

            auto double_array = std::static_pointer_cast<arrow::DoubleArray>(float64_array);
            for (int64_t row_in_chunk = 0; row_in_chunk < double_array->length(); ++row_in_chunk) {
                if (double_array->IsNull(row_in_chunk)) {
                    const int64_t global_row = row_offset + row_in_chunk;
                    throw std::runtime_error(
                        "Parquet contains null price value: file=" + parquet_path +
                        ", column=" + column_name +
                        ", row_index=" + std::to_string(global_row)
                    );
                }
                const int64_t global_row = row_offset + row_in_chunk;
                size_t matrix_index =
                    static_cast<size_t>(global_row) * static_cast<size_t>(number_of_columns) +
                    static_cast<size_t>(output_col_idx);
                closing_prices[matrix_index] = double_array->Value(row_in_chunk);
            }
            row_offset += double_array->length();
        }

        if (row_offset != number_of_rows) {
            throw std::runtime_error("Parquet column length mismatch");
        }
    }

    return closing_prices;
}
