#include "aliva/association/experiment.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace aliva::assoc {
namespace {

constexpr double kInputCurrent_mV_per_ms = 3.0;
constexpr std::uint64_t kRewardRngSalt = 0x9E3779B97F4A7C15ULL;

// Validate before any dependent members are initialized.
Config validated_config(Config config) {
    config.validate();
    return config;
}

HomeostasisParameters homeostasis_for_mode(const Config& config) {
    HomeostasisParameters parameters = config.homeostasis;
    parameters.enabled = (config.mode == Mode::RewardSTDPHomeostasis);
    return parameters;
}

// The environment's knowledge of the correct response. This is the only place
// the cue->response contingency is encoded; it is never wired into weights.
std::size_t correct_response(Cue cue, bool reversed) noexcept {
    const std::size_t base = (cue == Cue::A) ? 0u : 1u;
    return reversed ? (1u - base) : base;
}

double accuracy_of(const std::vector<TrialRecord>& records, Phase phase, std::size_t episode) {
    std::size_t correct = 0;
    std::size_t total = 0;
    for (const TrialRecord& rec : records) {
        if (rec.phase == phase && rec.episode == episode) {
            correct += rec.correct ? 1u : 0u;
            ++total;
        }
    }
    return total == 0 ? 0.0 : static_cast<double>(correct) / static_cast<double>(total);
}

double phase_accuracy(const std::vector<TrialRecord>& records, Phase phase) {
    std::size_t correct = 0;
    std::size_t total = 0;
    for (const TrialRecord& rec : records) {
        if (rec.phase == phase) {
            correct += rec.correct ? 1u : 0u;
            ++total;
        }
    }
    return total == 0 ? 0.0 : static_cast<double>(correct) / static_cast<double>(total);
}

} // namespace

std::string to_string(Phase phase) {
    switch (phase) {
        case Phase::Train: return "train";
        case Phase::Eval: return "eval";
        case Phase::ReversalTrain: return "reversal_train";
        case Phase::ReversalEval: return "reversal_eval";
        case Phase::Retention: return "retention";
    }
    return "unknown";
}

Experiment::Experiment(Config config, std::uint64_t seed)
    : config_(validated_config(std::move(config))),
      seed_(seed),
      rng_(seed),
      reward_rng_(seed ^ kRewardRngSalt),
      plasticity_(config_.plasticity),
      brain_(plasticity_, homeostasis_for_mode(config_)),
      patterns_(make_cue_patterns(
          config_.network.num_input,
          config_.network.active_inputs_per_cue,
          kInputCurrent_mV_per_ms,
          rng_)),
      handles_(build_network(
          brain_,
          config_.network,
          patterns_,
          config_.mode,
          rng_)) {
    summary_.mode = config_.mode;
    summary_.seed = seed_;
}

std::vector<Cue> Experiment::make_sequence(std::size_t length) {
    std::vector<Cue> sequence;
    sequence.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        sequence.push_back(cue_from_index(i % 2));
    }
    std::shuffle(sequence.begin(), sequence.end(), rng_);
    return sequence;
}

TrialRecord Experiment::run_trial(
    Cue cue, Phase phase, bool train, bool reversed, std::uint64_t episode)
{
    TrialRecord rec;
    rec.trial = records_.size();
    rec.episode = episode;
    rec.phase = phase;
    rec.cue = cue_index(cue);
    rec.correct_response = correct_response(cue, reversed);
    rec.sim_time_ms = brain_.simulation_time_ms();

    const double dt = config_.timing.dt_ms;
    std::vector<double> base_currents(brain_.neuron_count(), 0.0);
    for (const std::size_t index : patterns_.inputs(cue)) {
        base_currents[handles_.input[index]] = patterns_.input_current_mV_per_ms;
    }
    std::vector<double> currents = base_currents;
    std::vector<double> no_input(brain_.neuron_count(), 0.0);
    std::normal_distribution<double> noise{0.0, config_.timing.background_noise_mV_per_ms};
    const bool use_noise = config_.timing.background_noise_mV_per_ms > 0.0;

    std::size_t spikes0 = 0;
    std::size_t spikes1 = 0;
    std::size_t total_spikes = 0;
    int first_spike0 = -1;
    int first_spike1 = -1;

    // Cue presentation + read-out window.
    for (int t = 0; t < config_.timing.settle_steps; ++t) {
        currents = base_currents;
        if (use_noise) {
            for (double& current : currents) {
                current += noise(rng_);
            }
        }
        const StepResult result = brain_.step(dt, currents, 0.0);
        total_spikes += result.spikes.size();
        for (const NeuronId id : result.spikes) {
            if (id == handles_.output[0]) {
                if (first_spike0 < 0) first_spike0 = t;
                ++spikes0;
            } else if (id == handles_.output[1]) {
                if (first_spike1 < 0) first_spike1 = t;
                ++spikes1;
            }
        }
    }

    const double potential0 = brain_.neuron(handles_.output[0]).membrane_potential_mV();
    const double potential1 = brain_.neuron(handles_.output[1]).membrane_potential_mV();
    const DecodeResult decoded = decode_response(
        spikes0, spikes1, first_spike0, first_spike1, potential0, potential1);
    rec.selected_response = decoded.response;
    rec.output0_spikes = decoded.output0_spikes;
    rec.output1_spikes = decoded.output1_spikes;
    rec.correct = (decoded.response == rec.correct_response);

    // Reward / modulatory signal. In evaluation (train == false) the reward is
    // exactly 0 so that no weight can change; this enforces the train/eval
    // separation at the API level.
    double reward = 0.0;
    if (train) {
        if (config_.mode == Mode::OrdinarySTDP) {
            reward = 0.0; // Unmodulated STDP learns from timing, never task reward.
        } else if (config_.mode == Mode::ShuffledReward) {
            // Uninformative, exactly-balanced reward drawn from a per-phase
            // shuffled sequence (independent of the response).
            reward = shuffled_rewards_[shuffled_cursor_++];
        } else {
            // Fixed mode has no plastic synapses, so this value is logged for
            // comparability but cannot change any weight.
            reward = rec.correct ? config_.task.reward_correct
                                 : config_.task.reward_incorrect;
        }
    }
    rec.reward = reward;

    // Reward step(s): convert active eligibility into weight changes.
    for (int t = 0; t < config_.timing.reward_steps; ++t) {
        const StepResult result = brain_.step(dt, no_input, reward);
        total_spikes += result.spikes.size();
    }

    // Inter-trial interval: no input, no reward, lets the state settle.
    for (int t = 0; t < config_.timing.iti_steps; ++t) {
        const StepResult result = brain_.step(dt, no_input, 0.0);
        total_spikes += result.spikes.size();
    }

    rec.total_spikes = total_spikes;
    records_.push_back(rec);

    static const bool debug = std::getenv("ALIVA_DEBUG") != nullptr;
    if (debug) {
        std::cerr << "trial " << rec.trial << " phase " << to_string(phase)
                  << " cue " << (rec.cue == 0 ? 'A' : 'B')
                  << " s0 " << spikes0 << " s1 " << spikes1
                  << " f0 " << first_spike0 << " f1 " << first_spike1
                  << " sel " << rec.selected_response
                  << " reward " << rec.reward << '\n';
    }
    return rec;
}

void Experiment::run_phase(
    Phase phase, bool train, bool reversed, int episodes, int trials_per_episode)
{
    brain_.set_learning_enabled(train);
    brain_.set_homeostasis_enabled(train && config_.mode == Mode::RewardSTDPHomeostasis);
    // For the shuffled-reward control, build an exactly-balanced (+1/-1)
    // sequence for this training phase and shuffle it. This keeps the reward
    // statistics identical to the informative condition while making it
    // uncorrelated with the response.
    if (train && config_.mode == Mode::ShuffledReward) {
        const std::size_t total =
            static_cast<std::size_t>(episodes) * static_cast<std::size_t>(trials_per_episode);
        shuffled_rewards_.clear();
        shuffled_rewards_.reserve(total);
        for (std::size_t i = 0; i < total; ++i) {
            shuffled_rewards_.push_back((i % 2 == 0) ? config_.task.reward_correct
                                                    : config_.task.reward_incorrect);
        }
        std::shuffle(shuffled_rewards_.begin(), shuffled_rewards_.end(), reward_rng_);
        shuffled_cursor_ = 0;
    }

    for (int e = 0; e < episodes; ++e) {
        const std::vector<Cue> sequence = make_sequence(
            static_cast<std::size_t>(trials_per_episode));
        for (const Cue cue : sequence) {
            (void)run_trial(cue, phase, train, reversed, static_cast<std::uint64_t>(e));
        }
    }
}

void Experiment::run_retention() {
    brain_.set_learning_enabled(false);
    brain_.set_homeostasis_enabled(false);
    // No-training interval: the network is left idle (no input, no reward) so
    // that only passive membrane decay occurs. Synaptic weights cannot change
    // because no reward is applied.
    std::vector<double> no_input(brain_.neuron_count(), 0.0);
    for (int t = 0; t < config_.task.retention_idle_steps; ++t) {
        (void)brain_.step(config_.timing.dt_ms, no_input, 0.0);
    }
    run_phase(Phase::Retention, false, false, 1, config_.task.retention_trials);
}

void Experiment::run() {
    config_.validate();
    records_.clear();

    run_phase(Phase::Train, true, false,
              config_.task.train_episodes, config_.task.trials_per_episode);
    run_phase(Phase::Eval, false, false, 1, config_.task.eval_trials);

    run_retention();

    run_phase(Phase::ReversalTrain, true, true,
              config_.task.reversal_episodes, config_.task.reversal_trials_per_episode);
    run_phase(Phase::ReversalEval, false, true, 1, config_.task.reversal_eval_trials);

    finalize_summary();
}

std::vector<double> Experiment::hidden_to_output_weights() const {
    std::vector<double> weights;
    weights.reserve(handles_.hidden.size() * handles_.output.size());
    for (const NeuronId hidden : handles_.hidden) {
        for (const Synapse& synapse : brain_.synapses_from(hidden)) {
            if (synapse.target == handles_.output[0] || synapse.target == handles_.output[1]) {
                weights.push_back(synapse.weight_mV);
            }
        }
    }
    return weights;
}

void Experiment::finalize_summary() {
    Summary& s = summary_;
    s.mode = config_.mode;
    s.seed = seed_;
    s.homeostasis_target_firing_rate_hz = config_.homeostasis.target_firing_rate_hz;
    s.homeostasis_rate_time_constant_ms = config_.homeostasis.rate_time_constant_ms;
    s.homeostasis_adaptation_rate = config_.homeostasis.adaptation_rate_mV_per_ms_per_hz_second;
    s.homeostasis_max_abs_bias_mV_per_ms = config_.homeostasis.max_abs_bias_current_mV_per_ms;
    s.total_trials = records_.size();

    s.episode_accuracy.clear();
    for (int e = 0; e < config_.task.train_episodes; ++e) {
        s.episode_accuracy.push_back(accuracy_of(records_, Phase::Train, e));
    }
    s.train_accuracy = phase_accuracy(records_, Phase::Train);
    s.eval_accuracy = phase_accuracy(records_, Phase::Eval);

    s.trials_to_threshold = -1;
    for (int e = 0; e < config_.task.train_episodes; ++e) {
        if (s.episode_accuracy[static_cast<std::size_t>(e)] >= config_.task.accuracy_threshold) {
            s.trials_to_threshold =
                static_cast<int>((static_cast<std::size_t>(e) + 1u) *
                                 static_cast<std::size_t>(config_.task.trials_per_episode));
            break;
        }
    }

    s.reversal_episode_accuracy.clear();
    for (int e = 0; e < config_.task.reversal_episodes; ++e) {
        s.reversal_episode_accuracy.push_back(accuracy_of(records_, Phase::ReversalTrain, e));
    }
    s.reversal_eval_accuracy = phase_accuracy(records_, Phase::ReversalEval);
    s.retention_accuracy = phase_accuracy(records_, Phase::Retention);

    std::size_t total_spikes = 0;
    for (const TrialRecord& rec : records_) {
        total_spikes += rec.total_spikes;
    }
    s.total_spikes = total_spikes;
    s.neuron_count = brain_.neuron_count();
    s.synapse_count = brain_.synapse_count();

    double total_abs_homeostatic_bias = 0.0;
    for (std::size_t index = 0; index < brain_.neuron_count(); ++index) {
        total_abs_homeostatic_bias += std::abs(
            brain_.homeostatic_bias_current_mV_per_ms(static_cast<NeuronId>(index)));
    }
    if (brain_.neuron_count() > 0) {
        s.mean_abs_homeostatic_bias_mV_per_ms = total_abs_homeostatic_bias /
            static_cast<double>(brain_.neuron_count());
    }

    const double total_seconds = brain_.simulation_time_ms() / 1000.0;
    if (total_seconds > 0.0 && s.neuron_count > 0) {
        s.mean_firing_rate_hz =
            static_cast<double>(total_spikes) /
            (static_cast<double>(s.neuron_count) * total_seconds);
    }

    const std::vector<double> weights = hidden_to_output_weights();
    WeightStats& ws = s.output_weights;
    ws.count = weights.size();
    if (!weights.empty()) {
        const auto [min_it, max_it] = std::minmax_element(weights.begin(), weights.end());
        ws.min = *min_it;
        ws.max = *max_it;
        double sum = 0.0;
        double sum_abs = 0.0;
        for (const double w : weights) {
            sum += w;
            sum_abs += std::abs(w);
        }
        ws.mean = sum / static_cast<double>(weights.size());
        ws.mean_abs = sum_abs / static_cast<double>(weights.size());
    }
}

} // namespace aliva::assoc