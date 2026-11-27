#include "aliva/brain.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace aliva {
namespace {

constexpr double kEligibilityEpsilon = 1e-12;

void validate_plasticity(const PlasticityParameters& parameters) {
    const double values[] = {
        parameters.potentiation,
        parameters.depression,
        parameters.pre_trace_tau_ms,
        parameters.post_trace_tau_ms,
        parameters.eligibility_tau_ms,
        parameters.learning_rate_mV,
        parameters.max_abs_weight_mV
    };
    for (const double value : values) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Plasticity parameters must all be finite.");
        }
    }

    if (parameters.potentiation < 0.0 || parameters.depression < 0.0) {
        throw std::invalid_argument("STDP amplitudes cannot be negative.");
    }
    if (parameters.pre_trace_tau_ms <= 0.0 ||
        parameters.post_trace_tau_ms <= 0.0 ||
        parameters.eligibility_tau_ms <= 0.0) {
        throw std::invalid_argument("Plasticity time constants must be positive.");
    }
    if (parameters.learning_rate_mV < 0.0) {
        throw std::invalid_argument("Plasticity learning rate cannot be negative.");
    }
    if (parameters.max_abs_weight_mV <= 0.0) {
        throw std::invalid_argument("Maximum absolute synapse weight must be positive.");
    }
}

void validate_homeostasis(const HomeostasisParameters& parameters) {
    const double values[] = {
        parameters.target_firing_rate_hz,
        parameters.rate_time_constant_ms,
        parameters.adaptation_rate_mV_per_ms_per_hz_second,
        parameters.max_abs_bias_current_mV_per_ms
    };
    for (const double value : values) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Homeostasis parameters must all be finite.");
        }
    }

    if (parameters.target_firing_rate_hz <= 0.0) {
        throw std::invalid_argument("Homeostatic target firing rate must be positive.");
    }
    if (parameters.rate_time_constant_ms <= 0.0 ||
        !std::isfinite(1000.0 / parameters.rate_time_constant_ms)) {
        throw std::invalid_argument("Homeostatic rate time constant must be positive and representable.");
    }
    if (parameters.adaptation_rate_mV_per_ms_per_hz_second < 0.0) {
        throw std::invalid_argument("Homeostatic adaptation rate cannot be negative.");
    }
    if (parameters.max_abs_bias_current_mV_per_ms <= 0.0) {
        throw std::invalid_argument("Maximum homeostatic bias current must be positive.");
    }
}

} // namespace

Brain::Brain(PlasticityParameters plasticity, HomeostasisParameters homeostasis)
    : plasticity_(plasticity),
      homeostasis_(homeostasis),
      homeostasis_active_(homeostasis.enabled) {
    validate_plasticity(plasticity_);
    validate_homeostasis(homeostasis_);
}

NeuronId Brain::add_neuron(NeuronParameters parameters) {
    if (neurons_.size() >= static_cast<std::size_t>(std::numeric_limits<NeuronId>::max())) {
        throw std::length_error("Neuron ID space exhausted.");
    }

    const auto id = static_cast<NeuronId>(neurons_.size());
    neurons_.emplace_back(parameters);
    outgoing_synapses_.emplace_back();
    incoming_synapses_.emplace_back();
    pending_synaptic_kicks_mV_.push_back(0.0);
    pre_traces_.push_back(0.0);
    post_traces_.push_back(0.0);
    firing_rate_estimates_hz_.push_back(0.0);
    homeostatic_bias_currents_mV_per_ms_.push_back(0.0);
    return id;
}

void Brain::set_learning_enabled(bool enabled) noexcept {
    learning_enabled_ = enabled;
}

void Brain::set_homeostasis_enabled(bool enabled) noexcept {
    homeostasis_active_ = enabled && homeostasis_.enabled;
}

void Brain::connect(NeuronId source, NeuronId target, double weight_mV) {
    connect_impl(source, target, weight_mV, false, false);
}

void Brain::connect_stdp(NeuronId source, NeuronId target, double weight_mV) {
    if (weight_mV == 0.0) {
        throw std::invalid_argument("An STDP synapse needs a non-zero initial weight to define its polarity.");
    }
    if (std::abs(weight_mV) > plasticity_.max_abs_weight_mV) {
        throw std::invalid_argument("Initial STDP synapse weight exceeds the configured maximum.");
    }
    connect_impl(source, target, weight_mV, true, false);
}

void Brain::connect_plastic(NeuronId source, NeuronId target, double weight_mV) {
    if (weight_mV == 0.0) {
        throw std::invalid_argument("A plastic synapse needs a non-zero initial weight to define its polarity.");
    }
    if (std::abs(weight_mV) > plasticity_.max_abs_weight_mV) {
        throw std::invalid_argument("Initial plastic synapse weight exceeds the configured maximum.");
    }
    connect_impl(source, target, weight_mV, true, true);
}

void Brain::connect_impl(
    NeuronId source,
    NeuronId target,
    double weight_mV,
    bool plastic,
    bool reward_modulated
) {
    const auto source_index = static_cast<std::size_t>(source);
    const auto target_index = static_cast<std::size_t>(target);
    if (source_index >= neurons_.size() || target_index >= neurons_.size()) {
        throw std::out_of_range("Synapse endpoint does not exist.");
    }
    if (!std::isfinite(weight_mV)) {
        throw std::invalid_argument("Synapse weight must be finite.");
    }

    auto& outgoing = outgoing_synapses_[source_index];
    const auto edge_index = outgoing.size();
    const int polarity = weight_mV < 0.0 ? -1 : 1;
    outgoing.push_back(Synapse{
        target,
        weight_mV,
        0.0,
        plastic,
        reward_modulated,
        polarity,
        false
    });

    try {
        incoming_synapses_[target_index].push_back(
            SynapseRef{source, edge_index}
        );
    } catch (...) {
        outgoing.pop_back();
        throw;
    }

    ++synapse_count_;
}

void Brain::add_eligibility(const SynapseRef& ref, double delta) {
    auto& synapse = outgoing_synapses_[static_cast<std::size_t>(ref.source)][ref.index];
    if (!synapse.plastic) {
        return;
    }

    if (!std::isfinite(delta)) {
        throw std::overflow_error("STDP eligibility update overflow.");
    }

    // Ordinary STDP expresses the timing-dependent change immediately.
    // Reward-modulated STDP stores the same timing signal as eligibility and
    // waits for a later modulatory reward.
    if (!synapse.reward_modulated) {
        const double weight_delta = plasticity_.learning_rate_mV * delta;
        if (!std::isfinite(weight_delta)) {
            throw std::overflow_error("STDP weight update overflow.");
        }
        const double updated_magnitude = std::clamp(
            std::abs(synapse.weight_mV) + weight_delta,
            0.0,
            plasticity_.max_abs_weight_mV
        );
        synapse.weight_mV = static_cast<double>(synapse.polarity) * updated_magnitude;
        return;
    }

    const double updated_eligibility = synapse.eligibility + delta;
    if (!std::isfinite(updated_eligibility)) {
        throw std::overflow_error("Eligibility trace overflow.");
    }
    synapse.eligibility = updated_eligibility;

    if (!synapse.eligibility_active &&
        std::abs(synapse.eligibility) >= kEligibilityEpsilon) {
        active_eligibility_synapses_.push_back(ref);
        synapse.eligibility_active = true;
    }
}

StepResult Brain::step(
    double dt_ms,
    std::span<const double> external_currents_mV_per_ms,
    double reward_signal
) {
    if (!std::isfinite(dt_ms) || dt_ms <= 0.0) {
        throw std::invalid_argument("Time step must be finite and positive.");
    }
    if (!std::isfinite(reward_signal)) {
        throw std::invalid_argument("Reward signal must be finite.");
    }
    if (!external_currents_mV_per_ms.empty() &&
        external_currents_mV_per_ms.size() != neurons_.size()) {
        throw std::invalid_argument("External-current count must match neuron count.");
    }
    for (const double current : external_currents_mV_per_ms) {
        if (!std::isfinite(current)) {
            throw std::invalid_argument("External currents must be finite.");
        }
    }
    if (!std::isfinite(simulation_time_ms_ + dt_ms)) {
        throw std::overflow_error("Simulation time overflow.");
    }

    const double pre_decay = std::exp(-dt_ms / plasticity_.pre_trace_tau_ms);
    const double post_decay = std::exp(-dt_ms / plasticity_.post_trace_tau_ms);
    const double eligibility_decay = std::exp(-dt_ms / plasticity_.eligibility_tau_ms);
    const double firing_rate_decay = std::exp(-dt_ms / homeostasis_.rate_time_constant_ms);
    const double spike_rate_increment = 1000.0 / homeostasis_.rate_time_constant_ms;

    for (double& trace : pre_traces_) {
        trace *= pre_decay;
    }
    for (double& trace : post_traces_) {
        trace *= post_decay;
    }

    // Only synapses that currently carry an eligibility trace are visited.
    // This keeps the plasticity bookkeeping proportional to recent activity
    // rather than scanning every synapse in the network on every step.
    for (std::size_t index = 0; index < active_eligibility_synapses_.size();) {
        const SynapseRef ref = active_eligibility_synapses_[index];
        auto& synapse = outgoing_synapses_[static_cast<std::size_t>(ref.source)][ref.index];
        synapse.eligibility *= eligibility_decay;

        if (std::abs(synapse.eligibility) < kEligibilityEpsilon) {
            synapse.eligibility = 0.0;
            synapse.eligibility_active = false;
            active_eligibility_synapses_[index] = active_eligibility_synapses_.back();
            active_eligibility_synapses_.pop_back();
        } else {
            ++index;
        }
    }

    // Deliver spikes from the previous simulation step. This keeps updates
    // synchronous and prevents neuron iteration order from affecting results.
    for (const NeuronId source : last_spikes_) {
        for (const Synapse& synapse : outgoing_synapses_[static_cast<std::size_t>(source)]) {
            pending_synaptic_kicks_mV_[static_cast<std::size_t>(synapse.target)] += synapse.weight_mV;
        }
    }

    std::vector<NeuronId> current_spikes;
    current_spikes.reserve(last_spikes_.size());

    for (std::size_t index = 0; index < neurons_.size(); ++index) {
        const double external_current = external_currents_mV_per_ms.empty()
            ? 0.0
            : external_currents_mV_per_ms[index];
        const double homeostatic_bias = homeostasis_active_
            ? homeostatic_bias_currents_mV_per_ms_[index]
            : 0.0;
        const double effective_current = external_current + homeostatic_bias;
        if (!std::isfinite(effective_current)) {
            throw std::overflow_error("Combined external and homeostatic current overflow.");
        }

        const bool fired = neurons_[index].integrate(
            dt_ms,
            effective_current,
            pending_synaptic_kicks_mV_[index]
        );
        pending_synaptic_kicks_mV_[index] = 0.0;

        // Exponentially filtered spike count (Hz). The impulse increment is
        // 1000/tau_ms, making the steady-state estimate track spikes/second.
        double& rate_estimate = firing_rate_estimates_hz_[index];
        rate_estimate *= firing_rate_decay;
        if (fired) {
            rate_estimate += spike_rate_increment;
            current_spikes.push_back(static_cast<NeuronId>(index));
        }

        if (!std::isfinite(rate_estimate)) {
            throw std::overflow_error("Firing-rate estimate overflow.");
        }

        if (homeostasis_active_) {
            const double rate_error = homeostasis_.target_firing_rate_hz - rate_estimate;
            const double bias_delta =
                homeostasis_.adaptation_rate_mV_per_ms_per_hz_second *
                rate_error * (dt_ms / 1000.0);
            if (!std::isfinite(bias_delta)) {
                throw std::overflow_error("Homeostatic bias update overflow.");
            }

            double& bias = homeostatic_bias_currents_mV_per_ms_[index];
            bias = std::clamp(
                bias + bias_delta,
                -homeostasis_.max_abs_bias_current_mV_per_ms,
                homeostasis_.max_abs_bias_current_mV_per_ms
            );
        }
    }

    if (learning_enabled_) {
        // Pair-based STDP updates eligibility from traces accumulated before the
        // current step. Simultaneous pre/post spikes therefore do not get an
        // artificial ordering advantage.
        for (const NeuronId source : current_spikes) {
            const auto& outgoing = outgoing_synapses_[static_cast<std::size_t>(source)];
            for (std::size_t edge_index = 0; edge_index < outgoing.size(); ++edge_index) {
                const Synapse& synapse = outgoing[edge_index];
                if (synapse.plastic) {
                    const double post_trace = post_traces_[static_cast<std::size_t>(synapse.target)];
                    add_eligibility(
                        SynapseRef{source, edge_index},
                        -plasticity_.depression * post_trace
                    );
                }
            }
        }
    
        for (const NeuronId target : current_spikes) {
            for (const SynapseRef& ref : incoming_synapses_[static_cast<std::size_t>(target)]) {
                const double pre_trace = pre_traces_[static_cast<std::size_t>(ref.source)];
                add_eligibility(ref, plasticity_.potentiation * pre_trace);
            }
        }
    
        // A spike contributes to both a neuron's outgoing (pre) and incoming
        // (post) timing histories, after all pair comparisons for this step.
        for (const NeuronId id : current_spikes) {
            const auto index = static_cast<std::size_t>(id);
            pre_traces_[index] += 1.0;
            post_traces_[index] += 1.0;
        }
    
    
    }

    // Reward is applied to recent eligible events. Eligibility is retained
    // (and decays) after reinforcement, allowing multiple modulatory signals
    // during its lifetime. Weight polarity is fixed by the initial synapse.
    if (learning_enabled_ && reward_signal != 0.0 && plasticity_.learning_rate_mV != 0.0) {
        for (const SynapseRef& ref : active_eligibility_synapses_) {
            auto& synapse = outgoing_synapses_[static_cast<std::size_t>(ref.source)][ref.index];
            const double delta = plasticity_.learning_rate_mV *
                reward_signal * synapse.eligibility;
            if (!std::isfinite(delta)) {
                throw std::overflow_error("Plasticity weight update overflow.");
            }

            const double magnitude = std::abs(synapse.weight_mV);
            const double updated_magnitude = std::clamp(
                magnitude + delta,
                0.0,
                plasticity_.max_abs_weight_mV
            );
            synapse.weight_mV = static_cast<double>(synapse.polarity) * updated_magnitude;
        }
    }

    last_spikes_ = current_spikes;
    ++step_;
    simulation_time_ms_ += dt_ms;

    return StepResult{step_, simulation_time_ms_, std::move(current_spikes)};
}

std::size_t Brain::neuron_count() const noexcept {
    return neurons_.size();
}

std::size_t Brain::synapse_count() const noexcept {
    return synapse_count_;
}

double Brain::simulation_time_ms() const noexcept {
    return simulation_time_ms_;
}

double Brain::estimated_firing_rate_hz(NeuronId id) const {
    const auto index = static_cast<std::size_t>(id);
    if (index >= firing_rate_estimates_hz_.size()) {
        throw std::out_of_range("Neuron ID does not exist.");
    }
    return firing_rate_estimates_hz_[index];
}

double Brain::homeostatic_bias_current_mV_per_ms(NeuronId id) const {
    const auto index = static_cast<std::size_t>(id);
    if (index >= homeostatic_bias_currents_mV_per_ms_.size()) {
        throw std::out_of_range("Neuron ID does not exist.");
    }
    return homeostatic_bias_currents_mV_per_ms_[index];
}

const Neuron& Brain::neuron(NeuronId id) const {
    const auto index = static_cast<std::size_t>(id);
    if (index >= neurons_.size()) {
        throw std::out_of_range("Neuron ID does not exist.");
    }
    return neurons_[index];
}

std::span<const Synapse> Brain::synapses_from(NeuronId source) const {
    const auto index = static_cast<std::size_t>(source);
    if (index >= outgoing_synapses_.size()) {
        throw std::out_of_range("Neuron ID does not exist.");
    }
    return outgoing_synapses_[index];
}

const std::vector<NeuronId>& Brain::last_spikes() const noexcept {
    return last_spikes_;
}

} // namespace aliva
