#include "diffusion.hpp"

Vector diffuse_returns(
    const Matrix& laplacian,
    const Vector& latest_return,
    uint32_t number_of_assets,
    double diffusion_eta
) {
    // TODO: Implement s_t = exp(-eta * L) * r_t using eigendecomposition
    throw std::runtime_error("Not implemented");
}

double compute_systemic_ratio(
    const Vector& smoothed_return,
    const Vector& latest_return
) {
    // TODO: Implement Sys(t) = ||s_t||_2 / ||r_t||_2
    throw std::runtime_error("Not implemented");
}
