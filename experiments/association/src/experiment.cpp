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

// Validate before any dependent members (plasticity, brain, patterns, network)
// are initialized. Member initialization follows declaration order.
Config validated_config(Config config) {
    config.validate();
    return config;
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
      plasticity_(config_.plasticity),
      brain_(plasticity_),
      patterns_(make_cue_patterns(
          config_.network.num_input,
          config_.network.active_inputs_per_cue,
          kInputCurrent_mV_per_ms,
          rng_)),
      handles_(build_network(
          brain_,
          config_.network,
          patterns_,
          config_.mode != Mode::Fixed,
          rng_)) {
    summary_.mode = config_.mode;
    summary_.seed = seed_;
}
