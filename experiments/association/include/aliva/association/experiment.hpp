#pragma once

#include "aliva/association/config.hpp"
#include "aliva/association/decoding.hpp"
#include "aliva/association/network.hpp"
#include "aliva/association/patterns.hpp"
#include "aliva/brain.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace aliva::assoc {

// Coarse phase label attached to every recorded trial.
enum class Phase { Train, Eval, ReversalTrain, ReversalEval, Retention };

[[nodiscard]] std::string to_string(Phase phase);

// One recorded trial. This is the unit of the exported results file.
struct TrialRecord {
    std::uint64_t trial{0};       // global 0-based trial counter
    std::uint64_t episode{0};     // episode index within the phase
    Phase phase{Phase::Train};
    std::size_t cue{0};           // 0 = cue A, 1 = cue B
    std::size_t correct_response{0};
    std::size_t selected_response{0};
    double reward{0.0};
    bool correct{false};
    std::size_t output0_spikes{0};
    std::size_t output1_spikes{0};
    std::size_t total_spikes{0};
    double sim_time_ms{0.0};
};

struct WeightStats {
    std::size_t count{0};
    double min{0.0};
    double max{0.0};
    double mean{0.0};
    double mean_abs{0.0};
};

// Aggregate metrics computed from a completed run.
struct Summary {
    Mode mode{Mode::Fixed};
    std::uint64_t seed{0};

    std::vector<double> episode_accuracy;       // per training episode
    double train_accuracy{0.0};                 // pooled training accuracy
    double eval_accuracy{0.0};                  // held-out accuracy (acquisition)
    int trials_to_threshold{-1};                // first episode >= threshold, else -1

    std::vector<double> reversal_episode_accuracy;
    double reversal_eval_accuracy{0.0};

    double retention_accuracy{0.0};

    std::size_t total_trials{0};
    std::size_t total_spikes{0};
    double mean_firing_rate_hz{0.0};
    double mean_abs_homeostatic_bias_mV_per_ms{0.0};
    std::size_t neuron_count{0};
    std::size_t synapse_count{0};
    WeightStats output_weights;
};

// Drives the full associative-learning experiment on a single seeded brain.
//
// The lifecycle is: acquisition training -> held-out evaluation -> retention
// (no-training interval + evaluation) -> reversal training -> reversal
// evaluation. Evaluation phases never modify weights (reward is exactly 0).
class Experiment {
public:
    Experiment(Config config, std::uint64_t seed);

    // Execute the whole schedule and populate records()/summary().
    void run();

    [[nodiscard]] const std::vector<TrialRecord>& records() const noexcept { return records_; }
    [[nodiscard]] const Summary& summary() const noexcept { return summary_; }

    // Signed weights of every hidden->output synapse (row-major by hidden,
    // then output). Useful for tests and weight-distribution reporting.
    [[nodiscard]] std::vector<double> hidden_to_output_weights() const;

    [[nodiscard]] const Config& config() const noexcept { return config_; }
    [[nodiscard]] std::size_t neuron_count() const noexcept { return brain_.neuron_count(); }
    [[nodiscard]] std::size_t synapse_count() const noexcept { return brain_.synapse_count(); }

private:
    TrialRecord run_trial(Cue cue, Phase phase, bool train, bool reversed, std::uint64_t episode);
    [[nodiscard]] std::vector<Cue> make_sequence(std::size_t length);
    void run_phase(Phase phase, bool train, bool reversed, int episodes, int trials_per_episode);
    void run_retention();
    void finalize_summary();

    Config config_;
    std::uint64_t seed_;
    std::mt19937_64 rng_;
    PlasticityParameters plasticity_;
    Brain brain_;
    CuePatterns patterns_;
    NetworkHandles handles_;
    std::vector<TrialRecord> records_;
    Summary summary_;
    // Exactly-balanced, seeded-shuffled reward sequence for the shuffled-reward
    // control. Rebuilt per training phase; consumed one value per training trial.
    std::vector<double> shuffled_rewards_;
    std::size_t shuffled_cursor_{0};
};

} // namespace aliva::assoc