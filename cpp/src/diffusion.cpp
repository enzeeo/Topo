#include "diffusion.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

static void validate_square_matrix_size_or_throw(
    const Matrix& matrix_values,
    uint32_t number_of_assets,
    const std::string& matrix_name
) {
    const size_t expected_element_count =
        static_cast<size_t>(number_of_assets) * static_cast<size_t>(number_of_assets);

    if (matrix_values.size() != expected_element_count) {
        throw std::runtime_error(
            matrix_name + " must have size N*N. Got size=" +
            std::to_string(matrix_values.size()) +
            ", expected=" + std::to_string(expected_element_count) +
            ", N=" + std::to_string(number_of_assets)
        );
    }
}

static void validate_vector_size_or_throw(
    const Vector& vector_values,
    uint32_t number_of_assets,
    const std::string& vector_name
) {
    const size_t expected_element_count = static_cast<size_t>(number_of_assets);

    if (vector_values.size() != expected_element_count) {
        throw std::runtime_error(
            vector_name + " must have size N. Got size=" +
            std::to_string(vector_values.size()) +
            ", expected=" + std::to_string(expected_element_count) +
            ", N=" + std::to_string(number_of_assets)
        );
    }
}

static void validate_finite_or_throw(double value, const std::string& name) {
    if (std::isnan(value) || std::isinf(value)) {
        throw std::runtime_error(name + " must be finite (not NaN/Inf)");
    }
}

Vector diffuse_returns(
    const Matrix& laplacian,
    const Vector& latest_return,
    uint32_t number_of_assets,
    double diffusion_eta
) {
    validate_square_matrix_size_or_throw(laplacian, number_of_assets, "laplacian");
    validate_vector_size_or_throw(latest_return, number_of_assets, "latest_return");
    validate_finite_or_throw(diffusion_eta, "diffusion_eta");

    // Convert laplacian (row-major std::vector) -> Eigen matrix
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

    // Convert returns -> Eigen vector
    Eigen::VectorXd return_vector = Eigen::VectorXd::Zero(static_cast<int>(number_of_assets));
    for (uint32_t asset_index = 0; asset_index < number_of_assets; ++asset_index) {
        return_vector(static_cast<int>(asset_index)) = latest_return[static_cast<size_t>(asset_index)];
    }

    // Laplacian should be symmetric; use self-adjoint eigendecomposition.
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigen_solver(laplacian_matrix);
    if (eigen_solver.info() != Eigen::Success) {
        throw std::runtime_error("SelfAdjointEigenSolver failed for Laplacian matrix");
    }

    const Eigen::VectorXd eigenvalues = eigen_solver.eigenvalues();     // lambda_i
    const Eigen::MatrixXd eigenvectors = eigen_solver.eigenvectors();   // Q

    // Compute exp(-eta * L) * r = Q * exp(-eta * diag(lambda)) * Q^T * r
    // Step 1: y = Q^T * r
    const Eigen::VectorXd projected_returns = eigenvectors.transpose() * return_vector;

    // Step 2: scale each component by exp(-eta * lambda_i)
    Eigen::VectorXd scaled_projected_returns = projected_returns;
    for (int i = 0; i < scaled_projected_returns.size(); ++i) {
        const double lambda_value = eigenvalues(i);
        const double exponent_value = -diffusion_eta * lambda_value;
        const double scale_value = std::exp(exponent_value);

        scaled_projected_returns(i) *= scale_value;
    }

    // Step 3: s = Q * scaled
    const Eigen::VectorXd smoothed_vector = eigenvectors * scaled_projected_returns;

    // Convert back to std::vector
    Vector smoothed_return(static_cast<size_t>(number_of_assets), 0.0);
    for (uint32_t asset_index = 0; asset_index < number_of_assets; ++asset_index) {
        smoothed_return[static_cast<size_t>(asset_index)] =
            smoothed_vector(static_cast<int>(asset_index));
    }

    return smoothed_return;
}

double compute_systemic_ratio(
    const Vector& smoothed_return,
    const Vector& latest_return
) {
    if (smoothed_return.size() != latest_return.size()) {
        throw std::runtime_error("smoothed_return and latest_return must have the same length");
    }

    double smoothed_sum_squares = 0.0;
    double latest_sum_squares = 0.0;

    for (size_t i = 0; i < latest_return.size(); ++i) {
        const double smoothed_value = smoothed_return[i];
        const double latest_value = latest_return[i];

        if (std::isnan(smoothed_value) || std::isinf(smoothed_value)) {
            throw std::runtime_error("smoothed_return contains NaN/Inf");
        }
        if (std::isnan(latest_value) || std::isinf(latest_value)) {
            throw std::runtime_error("latest_return contains NaN/Inf");
        }

        smoothed_sum_squares += smoothed_value * smoothed_value;
        latest_sum_squares += latest_value * latest_value;
    }

    const double smoothed_norm = std::sqrt(smoothed_sum_squares);
    const double latest_norm = std::sqrt(latest_sum_squares);

    if (latest_norm == 0.0) {
        // If returns are all zero, define ratio as 0.0 to avoid division by zero.
        return 0.0;
    }

    return smoothed_norm / latest_norm;
}
