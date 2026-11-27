#include "aliva/brain.hpp"

#include <array>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct TestCase {
    const char* name;
    std::function<void()> run;
};

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Exception, typename Callable>
void require_throws(Callable&& callable, const std::string& message) {
    try {
        callable();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error(message);
}

aliva::PlasticityParameters fast_plasticity() {
    aliva::PlasticityParameters parameters;
    parameters.potentiation = 1.0;
    parameters.depression = 1.0;
    parameters.pre_trace_tau_ms = 1000.0;
    parameters.post_trace_tau_ms = 1000.0;
    parameters.eligibility_tau_ms = 1000.0;
    parameters.learning_rate_mV = 0.25;
    parameters.max_abs_weight_mV = 100.0;
    return parameters;
}

aliva::NeuronParameters fast_spiking_neuron() {
    aliva::NeuronParameters parameters;
    parameters.firing_threshold_mV = -60.0;
    parameters.refractory_period_ms = 10.0;
    return parameters;
}

void resting_neuron_does_not_spike() {
    aliva::Neuron neuron;
    for (int i = 0; i < 100; ++i) {
        require(!neuron.integrate(1.0), "A neuron at rest should not spike without input.");
    }
    require(std::abs(neuron.membrane_potential_mV() + 65.0) < 1e-9,
            "Resting potential should remain stable.");
}

void constant_current_eventually_causes_spike() {
    aliva::Neuron neuron;
    bool observed_spike = false;
    for (int i = 0; i < 30; ++i) {
        if (neuron.integrate(1.0, 2.0)) {
            observed_spike = true;
            break;
        }
    }
    require(observed_spike, "Sufficient sustained input should trigger a spike.");
    require(std::abs(neuron.membrane_potential_mV() + 65.0) < 1e-9,
            "A spiking neuron should reset its membrane potential.");
}

void synaptic_event_reaches_target_on_next_step() {
    aliva::Brain brain;
    aliva::NeuronParameters parameters;
    parameters.firing_threshold_mV = -60.0;
    parameters.refractory_period_ms = 0.0;

    const auto source = brain.add_neuron(parameters);
    const auto target = brain.add_neuron(parameters);
    brain.connect(source, target, 18.0);

    const std::array<double, 2> currents{3.0, 0.0};
    bool source_spiked = false;
    bool target_spiked = false;

    for (int tick = 0; tick < 8; ++tick) {
        const auto result = brain.step(1.0, currents);
        for (const auto id : result.spikes) {
            source_spiked = source_spiked || id == source;
            target_spiked = target_spiked || id == target;
        }
        if (target_spiked) {
            break;
        }
    }

    require(source_spiked, "The driven source neuron should spike.");
    require(target_spiked, "A sufficiently strong synapse should trigger the target neuron.");
}

void neurons_can_be_added_after_simulation_starts() {
    aliva::Brain brain;
    const auto first = brain.add_neuron();
    (void)brain.step(1.0);
    const auto second = brain.add_neuron();
    brain.connect(first, second, 1.0);

    require(second == 1, "Neuron IDs should be stable and sequential.");
    require(brain.neuron_count() == 2, "Dynamic neuron creation should update the network.");
    require(brain.synapse_count() == 1, "Dynamic synapse creation should update the network.");
}

void invalid_inputs_are_rejected() {
    aliva::Neuron neuron;
    require_throws<std::invalid_argument>([&] { (void)neuron.integrate(0.0); },
        "A zero time step should be rejected.");
    require_throws<std::invalid_argument>([&] { (void)neuron.integrate(1.0, NAN); },
        "A non-finite current should be rejected.");

    aliva::Brain brain;
    const auto id = brain.add_neuron();
    require_throws<std::out_of_range>([&] { brain.connect(id, 99, 1.0); },
        "Connecting to a missing neuron should be rejected.");
    const std::array<double, 2> wrong_count{0.0, 0.0};
    require_throws<std::invalid_argument>([&] { (void)brain.step(1.0, wrong_count); },
        "A mismatched external-current vector should be rejected.");
    require_throws<std::invalid_argument>([&] {
        (void)brain.step(1.0, {}, std::numeric_limits<double>::infinity());
    }, "A non-finite reward signal should be rejected.");
}

void reward_modulated_stdp_potentiates_pre_before_post() {
    aliva::Brain brain(fast_plasticity());
    const auto source = brain.add_neuron(fast_spiking_neuron());
    const auto target = brain.add_neuron(fast_spiking_neuron());
    brain.connect_plastic(source, target, 18.0);

    const std::array<double, 2> drive_source{20.0, 0.0};
    const std::array<double, 2> no_drive{0.0, 0.0};

    const auto pre = brain.step(1.0, drive_source);
    require(pre.spikes.size() == 1 && pre.spikes.front() == source,
            "The presynaptic neuron should fire first.");

    const auto post = brain.step(1.0, no_drive, 1.0);
    require(post.spikes.size() == 1 && post.spikes.front() == target,
            "The synaptic event should cause the postsynaptic neuron to fire.");
    require(brain.synapses_from(source).front().weight_mV > 18.0,
            "Positive reward should potentiate a pre-before-post eligible synapse.");
}

void eligibility_trace_allows_delayed_reward() {
    aliva::Brain brain(fast_plasticity());
    const auto source = brain.add_neuron(fast_spiking_neuron());
    const auto target = brain.add_neuron(fast_spiking_neuron());
    brain.connect_plastic(source, target, 18.0);

    const std::array<double, 2> drive_source{20.0, 0.0};
    const std::array<double, 2> no_drive{0.0, 0.0};

    (void)brain.step(1.0, drive_source);
    (void)brain.step(1.0, no_drive, 0.0);
    const double before_reward = brain.synapses_from(source).front().weight_mV;
    (void)brain.step(1.0, no_drive, 1.0);
    const double after_reward = brain.synapses_from(source).front().weight_mV;

    require(after_reward > before_reward,
            "A reward arriving after the spike pair should use the decaying eligibility trace.");
}

void post_before_pre_pairing_produces_depression() {
    aliva::Brain brain(fast_plasticity());
    const auto source = brain.add_neuron(fast_spiking_neuron());
    const auto target = brain.add_neuron(fast_spiking_neuron());
    brain.connect_plastic(source, target, 18.0);

    const std::array<double, 2> drive_target{0.0, 20.0};
    const std::array<double, 2> drive_source{20.0, 0.0};

    const auto post = brain.step(1.0, drive_target);
    require(post.spikes.size() == 1 && post.spikes.front() == target,
            "The postsynaptic neuron should fire first.");
    (void)brain.step(1.0, drive_source, 1.0);

    require(brain.synapses_from(source).front().weight_mV < 18.0,
            "Positive reward should express the depression eligibility of a post-before-pre pair.");
}

void fixed_synapses_do_not_learn() {
    aliva::Brain brain(fast_plasticity());
    const auto source = brain.add_neuron(fast_spiking_neuron());
    const auto target = brain.add_neuron(fast_spiking_neuron());
    brain.connect(source, target, 18.0);

    const std::array<double, 2> drive_source{20.0, 0.0};
    const std::array<double, 2> no_drive{0.0, 0.0};
    (void)brain.step(1.0, drive_source);
    (void)brain.step(1.0, no_drive, 1.0);

    require(std::abs(brain.synapses_from(source).front().weight_mV - 18.0) < 1e-12,
            "Ordinary fixed synapses must not change under reward.");
}

void plastic_inhibitory_synapse_keeps_its_polarity() {
    aliva::Brain brain(fast_plasticity());
    const auto source = brain.add_neuron(fast_spiking_neuron());
    const auto target = brain.add_neuron(fast_spiking_neuron());
    brain.connect_plastic(source, target, -18.0);

    const std::array<double, 2> drive_source{20.0, 0.0};
    const std::array<double, 2> no_drive{0.0, 0.0};
    (void)brain.step(1.0, drive_source);
    (void)brain.step(1.0, no_drive, 1.0);

    const double learned_weight = brain.synapses_from(source).front().weight_mV;
    require(learned_weight < 0.0,
            "Plasticity must not silently turn an inhibitory synapse into an excitatory one.");
    require(std::abs(learned_weight) > 18.0,
            "Positive reward should strengthen the magnitude of the eligible inhibitory synapse.");
}

void invalid_plasticity_configuration_is_rejected() {
    aliva::PlasticityParameters parameters;
    parameters.eligibility_tau_ms = 0.0;
    require_throws<std::invalid_argument>([&] { aliva::Brain invalid(parameters); },
        "A zero eligibility time constant should be rejected.");

    aliva::Brain brain;
    const auto source = brain.add_neuron();
    const auto target = brain.add_neuron();
    require_throws<std::invalid_argument>([&] { brain.connect_plastic(source, target, 0.0); },
        "A plastic synapse with zero initial weight should be rejected.");
}

} // namespace

int main() {
    const std::vector<TestCase> tests{
        {"resting neuron remains stable", resting_neuron_does_not_spike},
        {"constant current triggers a spike", constant_current_eventually_causes_spike},
        {"synaptic event propagates", synaptic_event_reaches_target_on_next_step},
        {"network supports dynamic neuron creation", neurons_can_be_added_after_simulation_starts},
        {"invalid inputs are rejected", invalid_inputs_are_rejected},
        {"reward-modulated STDP potentiates", reward_modulated_stdp_potentiates_pre_before_post},
        {"eligibility supports delayed reward", eligibility_trace_allows_delayed_reward},
        {"post-before-pre pairing depresses", post_before_pre_pairing_produces_depression},
        {"fixed synapses remain fixed", fixed_synapses_do_not_learn},
        {"inhibitory polarity is preserved", plastic_inhibitory_synapse_keeps_its_polarity},
        {"invalid plasticity settings are rejected", invalid_plasticity_configuration_is_rejected},
    };

    std::size_t failures = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
        }
    }

    std::cout << tests.size() - failures << '/' << tests.size() << " tests passed.\n";
    return failures == 0 ? 0 : 1;
}
