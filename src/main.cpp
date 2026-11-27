#include "aliva/brain.hpp"

#include <array>
#include <iomanip>
#include <iostream>

int main() {
    aliva::Brain brain;

    aliva::NeuronParameters parameters;
    parameters.firing_threshold_mV = -60.0;
    parameters.refractory_period_ms = 2.0;

    const aliva::NeuronId sensory = brain.add_neuron(parameters);
    const aliva::NeuronId relay = brain.add_neuron(parameters);
    brain.connect(sensory, relay, 18.0);

    std::cout << "Aliva neural-core demo\n"
              << "Two LIF neurons connected by one synapse.\n"
              << "A steady input drives the first neuron; its spikes excite the second.\n\n";

    const std::array<double, 2> currents{3.0, 0.0};
    for (int tick = 0; tick < 12; ++tick) {
        const aliva::StepResult result = brain.step(1.0, currents);
        std::cout << "t=" << std::setw(2) << result.step
                  << " ms | V0=" << std::setw(7) << std::fixed << std::setprecision(2)
                  << brain.neuron(sensory).membrane_potential_mV()
                  << " mV | V1=" << std::setw(7)
                  << brain.neuron(relay).membrane_potential_mV() << " mV | spikes:";

        if (result.spikes.empty()) {
            std::cout << " none";
        } else {
            for (const aliva::NeuronId id : result.spikes) {
                std::cout << ' ' << id;
            }
        }
        std::cout << '\n';
    }

    std::cout << "\nNeurons: " << brain.neuron_count()
              << " | Synapses: " << brain.synapse_count()
              << " | Simulated time: " << brain.simulation_time_ms() << " ms\n";
    return 0;
}
