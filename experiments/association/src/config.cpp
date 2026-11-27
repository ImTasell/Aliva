#include "aliva/association/config.hpp"

#include <cmath>

namespace aliva::assoc {

std::string to_string(Mode mode) {
    switch (mode) {
        case Mode::Fixed: return "fixed";
        case Mode::RewardSTDP: return "reward_stdp";
        case Mode::ShuffledReward: return "shuffled_reward";
    }
    return "unknown";
}

void Config::validate() const {
    if (network.num_input < 2) {
        throw std::invalid_argument("network.num_input must be at least 2.");
    }
    if (network.num_input < 2 * network.active_inputs_per_cue) {
        throw std::invalid_argument(
            "network.num_input must be at least 2*active_inputs_per_cue so the "
            "two cues activate disjoint inputs.");
    }
    if (network.num_hidden == 0) {
        throw std::invalid_argument("network.num_hidden must be at least 1.");
    }
    if (network.num_output != 2) {
        throw std::invalid_argument("network.num_output must be exactly 2.");
    }
    if (network.active_inputs_per_cue == 0 ||
        network.active_inputs_per_cue > network.num_input) {
        throw std::invalid_argument(
            "network.active_inputs_per_cue must be in [1, num_input].");
    }
    if (!(network.input_to_hidden_prob > 0.0 && network.input_to_hidden_prob <= 1.0)) {
        throw std::invalid_argument("network.input_to_hidden_prob must be in (0, 1].");
    }
    if (!std::isfinite(network.input_to_hidden_weight_mV) ||
        network.input_to_hidden_weight_mV <= 0.0) {
        throw std::invalid_argument("network.input_to_hidden_weight_mV must be positive.");
    }
    if (!std::isfinite(network.hidden_to_output_weight_mV) ||
        network.hidden_to_output_weight_mV <= 0.0) {
        throw std::invalid_argument("network.hidden_to_output_weight_mV must be positive.");
    }
    if (!std::isfinite(network.hidden_to_output_weight_jitter) ||
        network.hidden_to_output_weight_jitter < 0.0 ||
        network.hidden_to_output_weight_jitter >= 1.0) {
        throw std::invalid_argument(
            "network.hidden_to_output_weight_jitter must be in [0, 1).");
    }
    if (network.output_mutual_inhibition &&
        (!std::isfinite(network.output_inhibition_weight_mV) ||
         network.output_inhibition_weight_mV < 0.0)) {
        throw std::invalid_argument(
            "network.output_inhibition_weight_mV cannot be negative.");
    }

    if (!std::isfinite(timing.dt_ms) || timing.dt_ms <= 0.0) {
        throw std::invalid_argument("timing.dt_ms must be positive.");
    }
    if (timing.settle_steps <= 0) {
        throw std::invalid_argument("timing.settle_steps must be positive.");
    }
    if (timing.reward_steps <= 0) {
        throw std::invalid_argument("timing.reward_steps must be positive.");
    }
    if (timing.iti_steps < 0) {
        throw std::invalid_argument("timing.iti_steps cannot be negative.");
    }

    if (task.train_episodes <= 0 || task.trials_per_episode <= 0) {
        throw std::invalid_argument("Training episodes/trials must be positive.");
    }
    if (task.eval_trials <= 0) {
        throw std::invalid_argument("task.eval_trials must be positive.");
    }
    if (task.reversal_episodes <= 0 || task.reversal_trials_per_episode <= 0) {
        throw std::invalid_argument("Reversal episodes/trials must be positive.");
    }
    if (task.reversal_eval_trials <= 0) {
        throw std::invalid_argument("task.reversal_eval_trials must be positive.");
    }
    if (task.retention_trials <= 0) {
        throw std::invalid_argument("task.retention_trials must be positive.");
    }
    if (task.retention_idle_steps < 0) {
        throw std::invalid_argument("task.retention_idle_steps cannot be negative.");
    }
    if (!(task.accuracy_threshold > 0.0 && task.accuracy_threshold <= 1.0)) {
        throw std::invalid_argument("task.accuracy_threshold must be in (0, 1].");
    }

    // Plasticity parameters are validated by the Brain constructor, but check
    // the ones the harness relies on so failures surface early with context.
    if (!std::isfinite(plasticity.learning_rate_mV) || plasticity.learning_rate_mV < 0.0) {
        throw std::invalid_argument("plasticity.learning_rate_mV cannot be negative.");
    }
}

} // namespace aliva::assoc