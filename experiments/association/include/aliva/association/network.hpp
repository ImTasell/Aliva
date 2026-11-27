#pragma once

#include "aliva/association/config.hpp"
#include "aliva/association/patterns.hpp"
#include "aliva/brain.hpp"

#include <array>
#include <cstddef>
#include <random>
#include <vector>

namespace aliva::assoc {

// Stable handles to the populations created by build_network().
struct NetworkHandles {
    std::vector<NeuronId> input;
    std::vector<NeuronId> hidden;
    std::array<NeuronId, 2> output{};

    [[nodiscard]] std::size_t size() const noexcept {
        return input.size() + hidden.size() + output.size();
    }
};

// Build the synthetic circuit inside `brain`:
//
//   input  --fixed-->  hidden  --plastic/fixed-->  output
//
// Input->hidden connectivity is sparse and fixed (random but seeded). Each
// hidden neuron is assigned to a single cue channel (a "labeled line"): it
// samples inputs from only one cue's sensory pattern. This gives the two cues
// disjoint Kenyon-cell-like populations, a strong form of pattern separation
// that keeps the two associations from interfering. The hidden->output
// synapses are plastic when `plastic_output` is true (reward / shuffled modes)
// and fixed otherwise (fixed baseline), and each hidden neuron projects to a
// single output neuron (divergent read-out).
//
// The cue-to-response mapping is never wired in: only the environment's reward
// rule knows it.
[[nodiscard]] NetworkHandles build_network(
    Brain& brain,
    const NetworkConfig& config,
    const CuePatterns& patterns,
    bool plastic_output,
    std::mt19937_64& rng);

} // namespace aliva::assoc