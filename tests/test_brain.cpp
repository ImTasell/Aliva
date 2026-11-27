#include "aliva/brain.hpp"

#include <array>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
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
}

} // namespace

int main() {
    const std::vector<TestCase> tests{
        {"resting neuron remains stable", resting_neuron_does_not_spike},
        {"constant current triggers a spike", constant_current_eventually_causes_spike},
        {"synaptic event propagates", synaptic_event_reaches_target_on_next_step},
        {"network supports dynamic neuron creation", neurons_can_be_added_after_simulation_starts},
        {"invalid inputs are rejected", invalid_inputs_are_rejected},
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
