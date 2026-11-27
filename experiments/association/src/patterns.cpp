#include "aliva/association/patterns.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace aliva::assoc {

CuePatterns make_cue_patterns(
    std::size_t num_input,
    std::size_t active_per_cue,
    double input_current_mV_per_ms,
    std::mt19937_64& rng)
{
    if (num_input < 2) {
        throw std::invalid_argument("At least two input neurons are required.");
    }
    if (active_per_cue == 0 || active_per_cue > num_input) {
        throw std::invalid_argument("active_per_cue must be in [1, num_input].");
    }
    if (num_input < 2 * active_per_cue) {
        throw std::invalid_argument(
            "num_input must be at least 2*active_per_cue so the two cues can "
            "activate disjoint input neurons.");
    }

    std::vector<std::size_t> order(num_input);
    std::iota(order.begin(), order.end(), std::size_t{0});

    CuePatterns patterns;
    patterns.input_current_mV_per_ms = input_current_mV_per_ms;

    // Cue A: a random subset of the input neurons.
    std::shuffle(order.begin(), order.end(), rng);
    auto& first = patterns.active_inputs[cue_index(Cue::A)];
    first.assign(order.begin(),
                 order.begin() + static_cast<std::ptrdiff_t>(active_per_cue));

    // Cue B: a disjoint random subset drawn from the remaining inputs. Making
    // the two patterns non-overlapping maximises the sensory separation the
    // downstream network can exploit.
    std::shuffle(order.begin() + static_cast<std::ptrdiff_t>(active_per_cue),
                 order.end(), rng);
    auto& second = patterns.active_inputs[cue_index(Cue::B)];
    second.assign(order.begin() + static_cast<std::ptrdiff_t>(active_per_cue),
                  order.begin() + static_cast<std::ptrdiff_t>(2 * active_per_cue));

    return patterns;
}

} // namespace aliva::assoc