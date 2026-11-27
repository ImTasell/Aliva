#pragma once

#include <array>
#include <cstddef>
#include <random>
#include <vector>

namespace aliva::assoc {

// The two discrete cues presented to the network.
enum class Cue { A = 0, B = 1 };

[[nodiscard]] constexpr std::size_t cue_index(Cue cue) noexcept {
    return static_cast<std::size_t>(cue);
}

[[nodiscard]] constexpr Cue cue_from_index(std::size_t index) noexcept {
    return index == 0 ? Cue::A : Cue::B;
}

// A pair of distinct sensory input patterns, one per cue. Each pattern is the
// set of input-neuron indices that receive external current while the cue is
// presented, plus the current amplitude (mV/ms) applied to those neurons.
struct CuePatterns {
    std::array<std::vector<std::size_t>, 2> active_inputs;
    double input_current_mV_per_ms{3.0};

    [[nodiscard]] const std::vector<std::size_t>& inputs(Cue cue) const noexcept {
        return active_inputs[cue_index(cue)];
    }
};

// Deterministically draw two distinct input patterns from `rng`. The two cues
// are guaranteed to activate different input neurons so the network can, in
// principle, separate them. All randomness flows from the caller's seeded
// generator, so the same seed reproduces the same patterns.
[[nodiscard]] CuePatterns make_cue_patterns(
    std::size_t num_input,
    std::size_t active_per_cue,
    double input_current_mV_per_ms,
    std::mt19937_64& rng);

} // namespace aliva::assoc