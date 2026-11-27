# Initial neural-core architecture

## Scope

Version 0.1 establishes a small, deterministic spiking-network simulator. It is a foundation for research, not a claim that a useful general intelligence or a biological fly has been reproduced.

## Neuron model

Each neuron uses a leaky integrate-and-fire (LIF) approximation. Between spikes, membrane voltage follows a leaky response toward a resting potential, with an external input current. Crossing the threshold emits a spike, resets the voltage, and starts a refractory interval.

The implementation uses normalized simulation units:

- membrane voltage and synaptic kicks are expressed in mV-like units;
- external current is expressed in mV/ms-like units;
- time is expressed in milliseconds.

These values are convenient model units and are not fitted to a particular cell type.

## Synapses and update order

The network stores outgoing synapses per source neuron. A synapse contains a target neuron ID and a signed voltage-kick weight. Positive weights are excitatory; negative weights are inhibitory.

Simulation is synchronous:

1. Spikes from the previous step enqueue synaptic kicks for their targets.
2. Every neuron integrates external input and queued synaptic input.
3. Newly emitted spikes become the source events for the next step.

This means a synaptic event has a one-step delay and the result does not depend on the order in which neurons are iterated.

## Structural changes

The core can append neurons and synapses at runtime. Neuron IDs are stable, sequential indices and are not reused. Deletion, pruning, growth policies, and learned synaptic updates are intentionally not implemented yet; these need explicit rules and tests before being added.

## Planned experiments

- Add deterministic tests for synaptic plasticity before selecting a default learning rule.
- Compare pair-based spike-timing-dependent plasticity with reward-modulated alternatives on small tasks.
- Add a growth controller that responds to sustained learning difficulty or unmet prediction error, with hard resource budgets and measurable acceptance criteria.
- Add persistence only after neuron, synapse, and learning state have stable serialization formats.
- Introduce language tasks as experiments, not as a claim that a spiking network will automatically acquire language.

## Non-goals for this milestone

- No imported fruit-fly connectome data yet.
- No GPU dependency, external ML framework, or mandatory parallel runtime.
- No claim of consciousness, sentience, or human-level language ability.
