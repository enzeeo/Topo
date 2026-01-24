#include "correlation.hpp"

#include <Eigen/Dense>
#include <fstream>
#include <stdexcept>
#include <cmath>

Matrix compute_correlation(
    const Matrix& returns,
    uint32_t number_of_assets,
    uint32_t window_length
) {
    // Validate inputs
    size_t expected_size = static_cast<size_t>(window_length) * number_of_assets;
    if (returns.size() != expected_size) {
        throw std::runtime_error("returns size mismatch: expected " + 
                                 std::to_string(expected_size) + 
                                 ", got " + std::to_string(returns.size()));
    }

    if (window_length < 2) {
        throw std::runtime_error("window_length must be at least 2 for correlation");
    }

    // Map our flat vector to Eigen matrix for efficient computation
    // returns is [window_length x number_of_assets] in row-major order
    // Eigen uses column-major by default, so we need to handle this carefully
    Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> 
        return_matrix(returns.data(), window_length, number_of_assets);

    // Step 1: Compute mean for each asset (column mean)
    Eigen::VectorXd asset_means = return_matrix.colwise().mean();

    // Step 2: Center the data (subtract mean from each column)
    Eigen::MatrixXd centered_returns = return_matrix.rowwise() - asset_means.transpose();

    // Step 3: Compute covariance matrix
    // Cov = (1/(m-1)) * X^T * X where X is the centered data
    // Using Eigen's efficient matrix multiplication
    Eigen::MatrixXd covariance_matrix = (centered_returns.transpose() * centered_returns) 
                                        / (window_length - 1);

    // Step 4: Compute standard deviations (sqrt of diagonal of covariance)
    Eigen::VectorXd standard_deviations = covariance_matrix.diagonal().array().sqrt();

    // Step 5: Compute correlation matrix
    // M_{ij} = Cov(i,j) / (std_i * std_j)
    Eigen::MatrixXd correlation_matrix(number_of_assets, number_of_assets);

    for (uint32_t i = 0; i < number_of_assets; ++i) {
        for (uint32_t j = 0; j < number_of_assets; ++j) {
            double denominator = standard_deviations(i) * standard_deviations(j);
            
            if (denominator > 1e-10) {
                correlation_matrix(i, j) = covariance_matrix(i, j) / denominator;
            } else {
                // Handle zero variance case
                correlation_matrix(i, j) = (i == j) ? 1.0 : 0.0;
            }
        }
    }

    // Ensure diagonal is exactly 1.0 (numerical precision)
    for (uint32_t i = 0; i < number_of_assets; ++i) {
        correlation_matrix(i, i) = 1.0;
    }

    // Convert Eigen matrix back to our flat vector format (row-major)
    Matrix result(number_of_assets * number_of_assets);
    for (uint32_t i = 0; i < number_of_assets; ++i) {
        for (uint32_t j = 0; j < number_of_assets; ++j) {
            size_t index = i * number_of_assets + j;
            result[index] = correlation_matrix(i, j);
        }
    }

    return result;
}

void save_correlation_bin(
    const Matrix& correlation_matrix,
    uint32_t number_of_assets,
    const std::string& output_path
) {
    std::ofstream output_file(output_path, std::ios::binary);
    if (!output_file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + output_path);
    }

    // Write header: number_of_assets (uint32)
    output_file.write(reinterpret_cast<const char*>(&number_of_assets), sizeof(uint32_t));

    // Write data: correlation matrix (double array)
    size_t data_size = correlation_matrix.size() * sizeof(double);
    output_file.write(reinterpret_cast<const char*>(correlation_matrix.data()), data_size);

    if (!output_file.good()) {
        throw std::runtime_error("Error writing to file: " + output_path);
    }

    output_file.close();
}
