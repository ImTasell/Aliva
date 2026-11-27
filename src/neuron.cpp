#include "aliva/neuron.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aliva {
namespace {

[[nodiscard]] bool finite(double value) noexcept {
    return std::isfinite(value);
}

void validate_parameters(const NeuronParameters& parameters) {
    if (!finite(parameters.resting_potential_mV) ||
        !finite(parameters.firing_threshold_mV) ||
        !finite(parameters.reset_potential_mV) ||
        !finite(parameters.membrane_time_constant_ms) ||
        !finite(parameters.refractory_period_ms)) {
        throw std::invalid_argument("Neuron parameters must all be finite.");
    }

    if (parameters.membrane_time_constant_ms <= 0.0) {
        throw std::invalid_argument("Membrane time constant must be positive.");
    }
    if (parameters.refractory_period_ms < 0.0) {
        throw std::invalid_argument("Refractory period cannot be negative.");
    }
    if (parameters.firing_threshold_mV <= parameters.reset_potential_mV ||
        parameters.firing_threshold_mV <= parameters.resting_potential_mV) {
        throw std::invalid_argument("Firing threshold must be above resting and reset potentials.");
    }
}

} // namespace

Neuron::Neuron(NeuronParameters parameters)
    : parameters_(parameters),
      membrane_potential_mV_(parameters.resting_potential_mV) {
    validate_parameters(parameters_);
}

bool Neuron::integrate(
    double dt_ms,
    double external_current_mV_per_ms,
    double synaptic_kick_mV
) {
    if (!finite(dt_ms) || dt_ms <= 0.0) {
        throw std::invalid_argument("Time step must be finite and positive.");
    }
    if (!finite(external_current_mV_per_ms) || !finite(synaptic_kick_mV)) {
        throw std::invalid_argument("Neuron inputs must be finite.");
    }

    if (refractory_remaining_ms_ > 0.0) {
        refractory_remaining_ms_ = std::max(0.0, refractory_remaining_ms_ - dt_ms);
        membrane_potential_mV_ = parameters_.reset_potential_mV;
        return false;
    }

    const double decay = std::exp(-dt_ms / parameters_.membrane_time_constant_ms);
    const double current_response = external_current_mV_per_ms *
        parameters_.membrane_time_constant_ms * (1.0 - decay);

    membrane_potential_mV_ = parameters_.resting_potential_mV +
        (membrane_potential_mV_ - parameters_.resting_potential_mV) * decay +
        current_response + synaptic_kick_mV;

    if (membrane_potential_mV_ < parameters_.firing_threshold_mV) {
        return false;
    }

    membrane_potential_mV_ = parameters_.reset_potential_mV;
    refractory_remaining_ms_ = parameters_.refractory_period_ms;
    return true;
}

const NeuronParameters& Neuron::parameters() const noexcept {
    return parameters_;
}

double Neuron::membrane_potential_mV() const noexcept {
    return membrane_potential_mV_;
}

double Neuron::refractory_remaining_ms() const noexcept {
    return refractory_remaining_ms_;
}

} // namespace aliva
