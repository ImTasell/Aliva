#pragma once

#include "aliva/brain.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace aliva::assoc {

// Experimental condition for the associative task.
//
//  * Fixed          - hidden->output synapses are fixed; no learning.
//  * OrdinarySTDP - hidden->output synapses learn directly from spike timing.
//  * RewardSTDP - eligibility is converted into weight change by task reward.
//  * RewardSTDPHomeostasis - reward-modulated STDP plus intrinsic-bias regulation.
//  * ShuffledReward - reward-modulated STDP with uninformative random reward.
enum class Mode { Fixed, OrdinarySTDP, RewardSTDP, RewardSTDPHomeostasis, ShuffledReward };

[[nodiscard]] std::string to_string(Mode mode);

// Topology and connectivity of the synthetic mushroom-body-inspired circuit.
struct NetworkConfig {
    std::size_t num_input{24};            // sensory neurons
    std::size_t num_hidden{160};          // Kenyon-cell-like sparse layer
    std::size_t num_output{2};            // response read-out neurons
    std::size_t active_inputs_per_cue{6}; // inputs driven by each cue

    double input_to_hidden_prob{0.50};    // fixed input->hidden connection prob.
    double input_to_hidden_weight_mV{6.0};
    double hidden_to_output_weight_mV{0.15}; // initial magnitude (plastic or fixed)
    // Seeded per-synapse jitter applied to the initial hidden->output weight,
    // as a fraction of hidden_to_output_weight_mV. 0.0 keeps the two output
    // neurons symmetric so the fixed baseline is unbiased and symmetry breaking
    // is left to the background noise.
    double hidden_to_output_weight_jitter{0.0};
    // Fixed reciprocal inhibition between the two output neurons, producing a
    // soft winner-take-all read-out. Improves credit assignment by ensuring
    // mainly one response is active per trial.
    bool output_mutual_inhibition{true};
    double output_inhibition_weight_mV{40.0};
};

// Simulation timing for one trial, in milliseconds / steps.
struct TimingConfig {
    double dt_ms{1.0};
    int settle_steps{50};  // cue presentation + read-out window
    int reward_steps{1};   // steps over which the modulatory signal is applied
    int iti_steps{60};     // inter-trial interval (no input, no reward)
    // Std-dev of zero-mean Gaussian background current (mV/ms) added to every
    // neuron during the read-out window. Provides seeded exploration so that
    // both response neurons occasionally win, which is required for the global
    // reward to reach both read-out pathways. 0.0 disables it.
    double background_noise_mV_per_ms{1.0};
};

// Task schedule and reward magnitudes.
struct TaskConfig {
    int train_episodes{8};
    int trials_per_episode{20};
    int eval_trials{40};

    int reversal_episodes{25};
    int reversal_trials_per_episode{20};
    int reversal_eval_trials{40};

    int retention_trials{40};
    int retention_idle_steps{200}; // no-training interval before retention test

    double accuracy_threshold{0.80}; // "learned" threshold for trials-to-threshold
    double reward_correct{1.0};
    double reward_incorrect{-1.0};
};

// Tuned default plasticity for the associative task. Potentiation/depression
// and trace time constants are the core defaults; the learning rate and weight
// bound are set so that reward-modulated STDP acquires the association while
// keeping the learned weight asymmetry bounded (which keeps the fixed baseline
// unbiased and reversal possible). These are model parameters, not fitted
// biological constants.
[[nodiscard]] inline PlasticityParameters default_plasticity() {
    PlasticityParameters parameters;
    parameters.learning_rate_mV = 1.0;
    parameters.max_abs_weight_mV = 0.3;
    return parameters;
}

struct Config {
    Mode mode{Mode::RewardSTDP};
    NetworkConfig network;
    TimingConfig timing;
    TaskConfig task;
    PlasticityParameters plasticity{default_plasticity()};
    HomeostasisParameters homeostasis;
    std::uint64_t seed{1};
    std::string label;

    // Validate that the configuration is internally consistent. Throws
    // std::invalid_argument on the first problem found.
    void validate() const;
};

} // namespace aliva::assoc