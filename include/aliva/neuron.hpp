#pragma once

namespace aliva {

struct NeuronParameters {
    double resting_potential_mV{-65.0};
    double firing_threshold_mV{-50.0};
    double reset_potential_mV{-65.0};
    double membrane_time_constant_ms{20.0};
    double refractory_period_ms{2.0};
};

// A reduced leaky integrate-and-fire neuron.
// Current and voltage units are normalized for simulation, not calibrated
// to a particular biological neuron.
class Neuron {
public:
    explicit Neuron(NeuronParameters parameters = {});

    // Integrate one time step. Synaptic input is delivered as an instantaneous
    // voltage kick in mV; external current is expressed in mV/ms.
    [[nodiscard]] bool integrate(
        double dt_ms,
        double external_current_mV_per_ms = 0.0,
        double synaptic_kick_mV = 0.0
    );

    [[nodiscard]] const NeuronParameters& parameters() const noexcept;
    [[nodiscard]] double membrane_potential_mV() const noexcept;
    [[nodiscard]] double refractory_remaining_ms() const noexcept;

private:
    NeuronParameters parameters_;
    double membrane_potential_mV_;
    double refractory_remaining_ms_{0.0};
};

} // namespace aliva
