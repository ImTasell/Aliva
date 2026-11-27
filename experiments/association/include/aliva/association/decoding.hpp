#pragma once

#include <cstddef>

namespace aliva::assoc {

// Result of reading out the network's response for a single trial.
struct DecodeResult {
    std::size_t response{0};       // selected response (0 or 1)
    std::size_t output0_spikes{0}; // spikes emitted by output neuron 0
    std::size_t output1_spikes{0}; // spikes emitted by output neuron 1
};

// Deterministically decode a response from the read-out of the two output
// neurons.
//
// The primary signal is first-spike latency: the output neuron that spikes
// earliest wins (earlier = stronger net drive). This is a graded, non-saturating
// code, unlike raw spike count which saturates at the refractory limit. Ties
// are resolved by (1) total spike count, then (2) the final membrane
// potential, then (3) response 0. `first_spike_*` is the settle-window step at
// which each output first spiked, or a negative value if it never spiked.
// `potential_*` is each output's membrane potential at the end of the window.
// Decoding is a pure, reproducible function of these observed values.
[[nodiscard]] inline DecodeResult decode_response(
    std::size_t output0_spikes,
    std::size_t output1_spikes,
    int first_spike0,
    int first_spike1,
    double potential0,
    double potential1) noexcept
{
    DecodeResult result;
    result.output0_spikes = output0_spikes;
    result.output1_spikes = output1_spikes;

    if (first_spike0 < 0 && first_spike1 < 0) {
        // Neither output spiked: fall back to the more depolarised neuron.
        result.response = (potential1 > potential0) ? std::size_t{1} : std::size_t{0};
    } else if (first_spike0 < 0) {
        result.response = 1;
    } else if (first_spike1 < 0) {
        result.response = 0;
    } else if (first_spike0 < first_spike1) {
        result.response = 0;
    } else if (first_spike1 < first_spike0) {
        result.response = 1;
    } else if (output1_spikes > output0_spikes) {
        result.response = 1;
    } else if (output0_spikes > output1_spikes) {
        result.response = 0;
    } else {
        result.response = (potential1 > potential0) ? std::size_t{1} : std::size_t{0};
    }
    return result;
}

} // namespace aliva::assoc