/**
 * @file test_io_parquet.cpp
 * @brief Test program for io_parquet functionality.
 *
 * Usage:
 *   ./test_io_parquet <path_to_parquet_file>
 *
 * Expected output:
 *   - Number of rows and columns read
 *   - First few values from the matrix
 *   - Summary statistics
 */

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

#include "types.hpp"
#include "io_parquet.hpp"

void print_parquet_columns(const std::string& path) {
    auto file_result = arrow::io::ReadableFile::Open(path);
    if (!file_result.ok()) return;
    
    parquet::arrow::FileReaderBuilder reader_builder;
    reader_builder.Open(file_result.ValueOrDie());
    
    std::unique_ptr<parquet::arrow::FileReader> reader;
    reader_builder.Build(&reader);
    
    std::shared_ptr<arrow::Table> table;
    reader->ReadTable(&table);
    
    std::cout << "Arrow sees " << table->num_columns() << " columns:" << std::endl;
    for (int i = 0; i < table->num_columns(); ++i) {
        std::cout << "  [" << i << "] \"" << table->field(i)->name() << "\"" << std::endl;
    }
    std::cout << std::string(60, '-') << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <parquet_file_path>" << std::endl;
        std::cerr << "Example: " << argv[0] << " ../../data/compute_inputs/prices_window.parquet" << std::endl;
        return 1;
    }

    std::string parquet_path = argv[1];
    std::cout << "Reading parquet file: " << parquet_path << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // First, print what columns Arrow sees
    print_parquet_columns(parquet_path);

    try {
        uint32_t number_of_rows = 0;
        uint32_t number_of_columns = 0;

        Matrix closing_prices = read_close_prices_parquet(
            parquet_path,
            number_of_rows,
            number_of_columns
        );

        std::cout << "Successfully read parquet file!" << std::endl;
        std::cout << "  Rows (trading days): " << number_of_rows << std::endl;
        std::cout << "  Columns (tickers):   " << number_of_columns << std::endl;
        std::cout << "  Total values:        " << closing_prices.size() << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        
        std::cout << "NOTE: If column count is off by 1, check if parquet has a date/index column." << std::endl;
        std::cout << "You can inspect with: python -c \"import pandas as pd; print(pd.read_parquet('<path>').columns.tolist()[:5])\"" << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Display first 5x5 corner of the matrix
        std::cout << "First 5x5 corner of price matrix:" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        
        int display_rows = std::min(5u, number_of_rows);
        int display_cols = std::min(5u, number_of_columns);
        
        for (int row = 0; row < display_rows; ++row) {
            std::cout << "  Row " << row << ": ";
            for (int col = 0; col < display_cols; ++col) {
                size_t index = row * number_of_columns + col;
                std::cout << std::setw(10) << closing_prices[index] << " ";
            }
            std::cout << "..." << std::endl;
        }
        std::cout << std::string(60, '-') << std::endl;

        // Contract check for this project setup:
        // - prices_window.parquet includes an extra "Date" column, ignored in C++
        // - number_of_rows should be rolling_window_length + 1 (e.g., 51 when rolling_window_length=50)
        std::cout << "Contract check:" << std::endl;
        if (number_of_rows == 51) {
            std::cout << "  Rows == 51: YES (GOOD)" << std::endl;
        } else {
            std::cout << "  Rows == 51: NO (CHECK)" << std::endl;
        }
        std::cout << std::string(60, '-') << std::endl;

        // Compute summary statistics
        double min_price = *std::min_element(closing_prices.begin(), closing_prices.end());
        double max_price = *std::max_element(closing_prices.begin(), closing_prices.end());
        
        double sum = 0.0;
        for (const double& price : closing_prices) {
            sum += price;
        }
        double mean_price = sum / closing_prices.size();

        std::cout << "Summary statistics:" << std::endl;
        std::cout << "  Min price:  " << min_price << std::endl;
        std::cout << "  Max price:  " << max_price << std::endl;
        std::cout << "  Mean price: " << mean_price << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Verify data integrity
        bool has_nan = false;
        bool has_negative = false;
        bool has_zero = false;
        
        for (const double& price : closing_prices) {
            if (std::isnan(price)) has_nan = true;
            if (price < 0) has_negative = true;
            if (price == 0) has_zero = true;
        }

        std::cout << "Data integrity checks:" << std::endl;
        std::cout << "  Contains NaN:      " << (has_nan ? "YES (BAD)" : "NO (GOOD)") << std::endl;
        std::cout << "  Contains negative: " << (has_negative ? "YES (BAD)" : "NO (GOOD)") << std::endl;
        std::cout << "  Contains zero:     " << (has_zero ? "YES (WARNING)" : "NO (GOOD)") << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        if (has_nan || has_negative) {
            std::cout << "TEST FAILED: Data integrity issues detected!" << std::endl;
            return 1;
        }

        std::cout << "TEST PASSED: io_parquet is working correctly!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
