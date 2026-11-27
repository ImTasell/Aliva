#pragma once

#include "aliva/neuron.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace aliva {

using NeuronId = std::uint32_t;

struct PlasticityParameters {
    // Pair-based STDP amplitudes. These are model parameters, not fitted
    // biological constants.
    double potentiation{0.01};
    double depression{0.012};
    double pre_trace_tau_ms{20.0};
    double post_trace_tau_ms{20.0};
    double eligibility_tau_ms{500.0};

    // Weight change per unit reward and eligibility.
    double learning_rate_mV{0.1};
    double max_abs_weight_mV{50.0};
};

struct Synapse {
    NeuronId target{};
    double weight_mV{};
    double eligibility{};
    bool plastic{false};
    int polarity{1};
    bool eligibility_active{false};
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
    explicit Brain(PlasticityParameters plasticity = {});

    [[nodiscard]] NeuronId add_neuron(NeuronParameters parameters = {});

    // Fixed synapse: its weight never changes.
    void connect(NeuronId source, NeuronId target, double weight_mV);

    // Plastic synapse: weight changes only when a finite, non-zero reward
    // signal is supplied to step() while this synapse has an eligibility trace.
    // Initial weight must be non-zero so its excitatory/inhibitory polarity is known.
    void connect_plastic(NeuronId source, NeuronId target, double weight_mV);

    // Pass either no currents (all zero) or one current per neuron.
    // reward_signal is a signed, dimensionless teaching/modulatory signal:
    // positive reinforces eligible timing patterns; negative punishes them.
    [[nodiscard]] StepResult step(
        double dt_ms,
        std::span<const double> external_currents_mV_per_ms = {},
        double reward_signal = 0.0
    );

    [[nodiscard]] std::size_t neuron_count() const noexcept;
    [[nodiscard]] std::size_t synapse_count() const noexcept;
    [[nodiscard]] double simulation_time_ms() const noexcept;
    [[nodiscard]] const Neuron& neuron(NeuronId id) const;
    [[nodiscard]] std::span<const Synapse> synapses_from(NeuronId source) const;
    [[nodiscard]] const std::vector<NeuronId>& last_spikes() const noexcept;

private:
    struct SynapseRef {
        NeuronId source{};
        std::size_t index{};
    };

    void connect_impl(NeuronId source, NeuronId target, double weight_mV, bool plastic);
    void add_eligibility(const SynapseRef& ref, double delta);

    PlasticityParameters plasticity_;
    std::vector<Neuron> neurons_;
    std::vector<std::vector<Synapse>> outgoing_synapses_;
    std::vector<std::vector<SynapseRef>> incoming_synapses_;
    std::vector<double> pending_synaptic_kicks_mV_;
    std::vector<double> pre_traces_;
    std::vector<double> post_traces_;
    std::vector<SynapseRef> active_eligibility_synapses_;
    std::vector<NeuronId> last_spikes_;
    std::size_t synapse_count_{0};
    std::uint64_t step_{0};
    double simulation_time_ms_{0.0};
};

} // namespace aliva
