# Initial neural-core architecture

## Scope

The first milestone establishes a small, deterministic spiking-network simulator. It is a foundation for research, not a claim that a useful general intelligence or a biological fly has been reproduced.

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

## Reward-modulated STDP

Fixed synapses created with \`connect()\` never change their weights. Plastic synapses created with \`connect_plastic()\` maintain a temporary eligibility trace.

The first learning rule is a pair-based, three-factor STDP approximation:

- a presynaptic spike following a recent postsynaptic spike adds a negative (depressing) eligibility contribution;
- a postsynaptic spike following a recent presynaptic spike adds a positive (potentiating) eligibility contribution;
- a signed reward signal converts the accumulated eligibility into a weight change.

Conceptually:

\`weight_change = learning_rate * reward_signal * eligibility_trace\`

Pre/post traces and eligibility decay exponentially. Reward may arrive after the spike pair, while eligibility remains. Excitatory/inhibitory polarity is fixed at synapse creation; learning changes magnitude only. This is an experimental model rule, not a claim to reproduce the exact biophysics or learning algorithm of the fly mushroom body.

Eligibility bookkeeping uses an active list so the engine decays traces only for recently eligible plastic synapses rather than scanning every synapse every step. Neuron traces are updated per neuron.

## Structural changes

The core can append neurons and synapses at runtime. Neuron IDs are stable, sequential indices and are not reused. Deletion, pruning, growth policies, and connection rewiring are not implemented yet; these need explicit rules and tests before being added.

## Planned experiments

- Compare ordinary pair-based STDP, reward-modulated STDP, and a fixed-weight baseline on simple associative tasks.
- Test delayed reward, causal versus anti-causal spike timing, reversal learning, and retention.
- Add structural plasticity only after a fixed-size network learns a simple association.
- Make growth proposals occur at simulation boundaries, with deterministic seeds and hard neuron/synapse budgets.
- Evaluate whether growth improves held-out performance, learning efficiency, and retention over a matched fixed-topology network.
- Add persistence only after neuron, synapse, and learning state have stable serialization formats.

## Non-goals for this milestone

- No imported fruit-fly connectome data yet.
- No GPU dependency, external ML framework, or mandatory parallel runtime.
- No claim of consciousness, sentience, or human-level language ability.
