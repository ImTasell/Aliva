#include "aliva/brain.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace aliva {

NeuronId Brain::add_neuron(NeuronParameters parameters) {
    if (neurons_.size() >= static_cast<std::size_t>(std::numeric_limits<NeuronId>::max())) {
        throw std::length_error("Neuron ID space exhausted.");
    }

    const auto id = static_cast<NeuronId>(neurons_.size());
    neurons_.emplace_back(parameters);
    outgoing_synapses_.emplace_back();
    pending_synaptic_kicks_mV_.push_back(0.0);
    return id;
}

void Brain::connect(NeuronId source, NeuronId target, double weight_mV) {
    const auto source_index = static_cast<std::size_t>(source);
    const auto target_index = static_cast<std::size_t>(target);
    if (source_index >= neurons_.size() || target_index >= neurons_.size()) {
        throw std::out_of_range("Synapse endpoint does not exist.");
    }
    if (!std::isfinite(weight_mV)) {
        throw std::invalid_argument("Synapse weight must be finite.");
    }

    outgoing_synapses_[source_index].push_back(Synapse{target, weight_mV});
    ++synapse_count_;
}

StepResult Brain::step(
    double dt_ms,
    std::span<const double> external_currents_mV_per_ms
) {
    if (!std::isfinite(dt_ms) || dt_ms <= 0.0) {
        throw std::invalid_argument("Time step must be finite and positive.");
    }
    if (!external_currents_mV_per_ms.empty() &&
        external_currents_mV_per_ms.size() != neurons_.size()) {
        throw std::invalid_argument("External-current count must match neuron count.");
    }
    for (const double current : external_currents_mV_per_ms) {
        if (!std::isfinite(current)) {
            throw std::invalid_argument("External currents must be finite.");
        }
    }
    if (!std::isfinite(simulation_time_ms_ + dt_ms)) {
        throw std::overflow_error("Simulation time overflow.");
    }

    // Deliver spikes from the previous simulation step. This keeps updates
    // synchronous and prevents neuron iteration order from affecting results.
    for (const NeuronId source : last_spikes_) {
        for (const Synapse& synapse : outgoing_synapses_[static_cast<std::size_t>(source)]) {
            pending_synaptic_kicks_mV_[static_cast<std::size_t>(synapse.target)] += synapse.weight_mV;
        }
    }

    std::vector<NeuronId> current_spikes;
    current_spikes.reserve(last_spikes_.size());

    for (std::size_t index = 0; index < neurons_.size(); ++index) {
        const double external_current = external_currents_mV_per_ms.empty()
            ? 0.0
            : external_currents_mV_per_ms[index];
        const bool fired = neurons_[index].integrate(
            dt_ms,
            external_current,
            pending_synaptic_kicks_mV_[index]
        );
        pending_synaptic_kicks_mV_[index] = 0.0;

        if (fired) {
            current_spikes.push_back(static_cast<NeuronId>(index));
        }
    }

    last_spikes_ = current_spikes;
    ++step_;
    simulation_time_ms_ += dt_ms;

    return StepResult{step_, simulation_time_ms_, std::move(current_spikes)};
}

std::size_t Brain::neuron_count() const noexcept {
    return neurons_.size();
}

std::size_t Brain::synapse_count() const noexcept {
    return synapse_count_;
}

double Brain::simulation_time_ms() const noexcept {
    return simulation_time_ms_;
}

const Neuron& Brain::neuron(NeuronId id) const {
    const auto index = static_cast<std::size_t>(id);
    if (index >= neurons_.size()) {
        throw std::out_of_range("Neuron ID does not exist.");
    }
    return neurons_[index];
}

const std::vector<NeuronId>& Brain::last_spikes() const noexcept {
    return last_spikes_;
}

} // namespace aliva
