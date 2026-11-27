#include "aliva/association/network.hpp"

#include <stdexcept>

namespace aliva::assoc {
namespace {

// A hidden neuron fires only if enough of its (fixed) sensory inputs are
// active, so the input->hidden weight must be large enough to cross threshold
// from rest. These are model parameters, not fitted biological constants.
constexpr double kHiddenThreshold_mV = -50.0;
constexpr double kHiddenRefractory_ms = 2.0;
constexpr double kOutputThreshold_mV = -50.0;
constexpr double kOutputRefractory_ms = 2.0;

} // namespace

NetworkHandles build_network(
    Brain& brain,
    const NetworkConfig& config,
    const CuePatterns& patterns,
    bool plastic_output,
    std::mt19937_64& rng)
{
    if (config.num_input < 2) {
        throw std::invalid_argument("Network needs at least two input neurons.");
    }
    if (config.num_hidden == 0) {
        throw std::invalid_argument("Network needs at least one hidden neuron.");
    }
    if (config.num_output != 2) {
        throw std::invalid_argument("This task requires exactly two output neurons.");
    }
    if (!(config.input_to_hidden_prob > 0.0 && config.input_to_hidden_prob <= 1.0)) {
        throw std::invalid_argument("input_to_hidden_prob must be in (0, 1].");
    }
    if (config.input_to_hidden_weight_mV <= 0.0 || config.hidden_to_output_weight_mV <= 0.0) {
        throw std::invalid_argument("Synapse weights must be positive magnitudes.");
    }

    NetworkHandles handles;

    aliva::NeuronParameters sensory;
    sensory.firing_threshold_mV = -55.0;
    sensory.refractory_period_ms = 2.0;
    handles.input.reserve(config.num_input);
    for (std::size_t i = 0; i < config.num_input; ++i) {
        handles.input.push_back(brain.add_neuron(sensory));
    }

    aliva::NeuronParameters hidden_params;
    hidden_params.firing_threshold_mV = kHiddenThreshold_mV;
    hidden_params.refractory_period_ms = kHiddenRefractory_ms;
    handles.hidden.reserve(config.num_hidden);
    for (std::size_t i = 0; i < config.num_hidden; ++i) {
        handles.hidden.push_back(brain.add_neuron(hidden_params));
    }

    aliva::NeuronParameters output_params;
    output_params.firing_threshold_mV = kOutputThreshold_mV;
    output_params.refractory_period_ms = kOutputRefractory_ms;
    for (std::size_t i = 0; i < config.num_output; ++i) {
        handles.output[static_cast<std::size_t>(i)] = brain.add_neuron(output_params);
    }

    std::uniform_real_distribution<double> uniform{0.0, 1.0};
    std::uniform_int_distribution<int> channel_choice{0, 1};

    // Fixed, sparse input->hidden connectivity with labeled-line assignment:
    // each hidden neuron samples inputs from exactly one cue channel.
    const std::vector<std::size_t>& inputs_a = patterns.inputs(Cue::A);
    const std::vector<std::size_t>& inputs_b = patterns.inputs(Cue::B);
    for (const NeuronId hidden : handles.hidden) {
        const std::vector<std::size_t>& channel =
            (channel_choice(rng) == 0) ? inputs_a : inputs_b;
        for (const std::size_t input_index : channel) {
            if (uniform(rng) < config.input_to_hidden_prob) {
                brain.connect(handles.input[input_index], hidden,
                              config.input_to_hidden_weight_mV);
            }
        }
    }

    // Hidden->output synapses: plastic (learnable) or fixed (baseline). Each
    // hidden neuron projects to a single, randomly chosen output neuron
    // (divergent read-out). The initial magnitude is jittered per synapse
    // (seeded) to break the symmetry between the two output neurons.
    const double jitter = config.hidden_to_output_weight_jitter;
    std::uniform_real_distribution<double> weight_scale{1.0 - jitter, 1.0 + jitter};
    for (const NeuronId hidden : handles.hidden) {
        const NeuronId output = handles.output[static_cast<std::size_t>(channel_choice(rng))];
        const double weight = config.hidden_to_output_weight_mV * weight_scale(rng);
        if (plastic_output) {
            brain.connect_plastic(hidden, output, weight);
        } else {
            brain.connect(hidden, output, weight);
        }
    }

    // Optional soft winner-take-all: each output inhibits the other.
    if (config.output_mutual_inhibition) {
        brain.connect(handles.output[0], handles.output[1], -config.output_inhibition_weight_mV);
        brain.connect(handles.output[1], handles.output[0], -config.output_inhibition_weight_mV);
    }

    return handles;
}

} // namespace aliva::assoc