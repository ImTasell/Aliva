#include "aliva/brain.hpp"

#include <array>
#include <iomanip>
#include <iostream>

int main() {
    aliva::PlasticityParameters learning;
    learning.potentiation = 0.02;
    learning.depression = 0.025;
    learning.learning_rate_mV = 0.2;
    learning.eligibility_tau_ms = 250.0;

    aliva::Brain brain(learning);

    aliva::NeuronParameters parameters;
    parameters.firing_threshold_mV = -60.0;
    parameters.refractory_period_ms = 2.0;

    const aliva::NeuronId sensory = brain.add_neuron(parameters);
    const aliva::NeuronId relay = brain.add_neuron(parameters);
    brain.connect_plastic(sensory, relay, 18.0);

    std::cout << "Aliva reward-modulated STDP demo\n"
              << "A sensory spike excites a relay neuron. A delayed reward reinforces\n"
              << "the eligible sensory-to-relay synapse.\n\n";

    const std::array<double, 2> currents{3.0, 0.0};
    double pending_reward = 0.0;

    for (int tick = 0; tick < 16; ++tick) {
        const aliva::StepResult result = brain.step(1.0, currents, pending_reward);
        pending_reward = 0.0;

        bool relay_fired = false;
        for (const aliva::NeuronId id : result.spikes) {
            std::cout << "t=" << std::setw(2) << result.step
                      << " ms | neuron " << id << " fired\n";
            relay_fired = relay_fired || id == relay;
        }

        // Deliver reinforcement on the following step, not at the instant
        // the output is observed. This demonstrates the eligibility trace.
        if (relay_fired) {
            pending_reward = 1.0;
            std::cout << "  output detected; reward scheduled for next step\n";
        }
    }

    const double learned_weight = brain.synapses_from(sensory).front().weight_mV;
    std::cout << "\nNeurons: " << brain.neuron_count()
              << " | Synapses: " << brain.synapse_count()
              << " | Learned synaptic weight: " << std::fixed << std::setprecision(4)
              << learned_weight << " mV\n";
    return 0;
}
