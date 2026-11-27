#pragma once

#include "aliva/neuron.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace aliva {

using NeuronId = std::uint32_t;

struct Synapse {
    NeuronId target{};
    double weight_mV{};
};

struct StepResult {
    std::uint64_t step{};
    double simulation_time_ms{};
    std::vector<NeuronId> spikes;
};

// A sparse, event-driven network. Spikes emitted during one step deliver
// synaptic voltage kicks at the beginning of the following step.
class Brain {
public:
    [[nodiscard]] NeuronId add_neuron(NeuronParameters parameters = {});
    void connect(NeuronId source, NeuronId target, double weight_mV);

    // Pass either no currents (all zero) or one current per neuron.
    [[nodiscard]] StepResult step(
        double dt_ms,
        std::span<const double> external_currents_mV_per_ms = {}
    );

    [[nodiscard]] std::size_t neuron_count() const noexcept;
    [[nodiscard]] std::size_t synapse_count() const noexcept;
    [[nodiscard]] double simulation_time_ms() const noexcept;
    [[nodiscard]] const Neuron& neuron(NeuronId id) const;
    [[nodiscard]] const std::vector<NeuronId>& last_spikes() const noexcept;

private:
    std::vector<Neuron> neurons_;
    std::vector<std::vector<Synapse>> outgoing_synapses_;
    std::vector<double> pending_synaptic_kicks_mV_;
    std::vector<NeuronId> last_spikes_;
    std::size_t synapse_count_{0};
    std::uint64_t step_{0};
    double simulation_time_ms_{0.0};
};

} // namespace aliva
