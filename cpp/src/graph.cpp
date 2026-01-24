#include "graph.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

static void validate_square_matrix_or_throw(
    const Matrix& matrix,
    uint32_t number_of_assets,
    const std::string& name
) {
    size_t expected_size =
        static_cast<size_t>(number_of_assets) * number_of_assets;

    if (matrix.size() != expected_size) {
        throw std::runtime_error(name + " must be N x N");
    }
}

static void validate_vector_or_throw(
    const Vector& vector,
    uint32_t number_of_assets,
    const std::string& name
) {
    if (vector.size() != number_of_assets) {
        throw std::runtime_error(name + " must have size N");
    }
}

Matrix build_weighted_graph(
    const Matrix& correlation_matrix,
    uint32_t number_of_assets
) {
    validate_square_matrix_or_throw(
        correlation_matrix,
        number_of_assets,
        "correlation_matrix"
    );

    Matrix weighted_adjacency(
        static_cast<size_t>(number_of_assets) * number_of_assets,
        0.0
    );

    for (uint32_t i = 0; i < number_of_assets; ++i) {
        for (uint32_t j = 0; j < number_of_assets; ++j) {
            size_t index = static_cast<size_t>(i) * number_of_assets + j;

            if (i == j) {
                weighted_adjacency[index] = 0.0;
            } else {
                weighted_adjacency[index] =
                    std::max(correlation_matrix[index], 0.0);
            }
        }
    }

    return weighted_adjacency;
}

Matrix compute_laplacian(
    const Matrix& weighted_adjacency,
    uint32_t number_of_assets
) {
    validate_square_matrix_or_throw(
        weighted_adjacency,
        number_of_assets,
        "weighted_adjacency"
    );

    Matrix laplacian(
        static_cast<size_t>(number_of_assets) * number_of_assets,
        0.0
    );

    for (uint32_t i = 0; i < number_of_assets; ++i) {
        double degree = 0.0;

        for (uint32_t j = 0; j < number_of_assets; ++j) {
            degree += weighted_adjacency[
                static_cast<size_t>(i) * number_of_assets + j
            ];
        }

        for (uint32_t j = 0; j < number_of_assets; ++j) {
            size_t index = static_cast<size_t>(i) * number_of_assets + j;

            if (i == j) {
                laplacian[index] = degree;
            } else {
                laplacian[index] = -weighted_adjacency[index];
            }
        }
    }

    return laplacian;
}

double compute_graph_total_variation(
    const Vector& latest_return,
    const Matrix& laplacian,
    uint32_t number_of_assets
) {
    validate_vector_or_throw(latest_return, number_of_assets, "latest_return");
    validate_square_matrix_or_throw(laplacian, number_of_assets, "laplacian");

    Vector laplacian_times_return(number_of_assets, 0.0);

    for (uint32_t i = 0; i < number_of_assets; ++i) {
        double sum = 0.0;
        for (uint32_t j = 0; j < number_of_assets; ++j) {
            sum += laplacian[
                static_cast<size_t>(i) * number_of_assets + j
            ] * latest_return[j];
        }
        laplacian_times_return[i] = sum;
    }

    double gtv = 0.0;
    for (uint32_t i = 0; i < number_of_assets; ++i) {
        gtv += latest_return[i] * laplacian_times_return[i];
    }

    return gtv;
}
