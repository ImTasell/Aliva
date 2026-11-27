# Aliva neural-core architecture

## Scope

Aliva is an experimental, connectome-inspired spiking-network simulator. The core is a foundation for controlled learning experiments, not a claim that a complete biological fly brain or general intelligence has been reproduced.

## Neuron model

Each neuron uses a leaky integrate-and-fire (LIF) approximation. Between spikes, membrane voltage follows a leaky response toward a resting potential, with an external input current. Crossing the threshold emits a spike, resets the voltage, and starts a refractory interval.

The implementation uses normalized simulation units:

- membrane voltage and synaptic kicks are expressed in mV-like units;
- external and homeostatic currents are expressed in mV/ms-like units;
- time is expressed in milliseconds.

These values are convenient model units and are not fitted to a particular cell type.

## Synapses and update order

The network stores outgoing synapses per source neuron and incoming references for plasticity updates. Each synapse has a target neuron ID and a signed voltage-kick weight. Positive weights are excitatory; negative weights are inhibitory.

Simulation is synchronous:

1. Spikes from the previous step enqueue synaptic kicks for their targets.
2. Every neuron integrates external current, optional homeostatic bias, and queued synaptic input.
3. Newly emitted spikes are used to update plasticity traces and become the source events for the next step.

This provides a one-step synaptic event delay and avoids dependence on neuron iteration order.

## Synaptic plasticity

Three connection modes are available:

- connect() creates a fixed synapse whose weight does not change.
- connect_stdp() creates an ordinary pair-based STDP synapse. Timing-dependent updates change its weight magnitude directly, without reward.
- connect_plastic() creates a reward-modulated STDP synapse. Timing creates an eligibility trace; a later signed reward signal converts that eligibility into a weight update.

The timing convention is:

- a presynaptic spike following a recent postsynaptic spike contributes a negative (depressing) timing term;
- a postsynaptic spike following a recent presynaptic spike contributes a positive (potentiating) timing term.

For ordinary STDP, the timing term immediately changes the synaptic weight magnitude. For reward-modulated STDP, the term accumulates in an eligibility trace and is converted by a modulatory signal:

weight_change = learning_rate * reward_signal * eligibility_trace

Pre/post traces and reward eligibility decay exponentially. Reward may arrive after the spike pair while eligibility remains. Excitatory/inhibitory polarity is fixed at synapse creation; plasticity changes magnitude only and weights are bounded.

This is an intentionally simple experimental pair-based model. It is not a claim to reproduce the exact biophysics or learning algorithm of the Drosophila mushroom body.

Reward eligibility uses an active list so the engine decays only recently eligible plastic synapses rather than scanning every synapse each step. Pre/post traces and firing-rate estimates are maintained per neuron.

## Homeostatic activity regulation

Homeostasis is optional and disabled by default. When enabled, the brain tracks an exponentially filtered firing-rate estimate for each neuron. The estimate decays with a configurable time constant and receives a spike impulse of 1000 / tau_ms Hz for each emitted spike.

A slow intrinsic-bias controller adjusts a per-neuron bias current:

bias += adaptation_rate * (target_rate - estimated_rate) * dt_seconds

The current is clamped to a configured maximum magnitude. Underactive neurons receive positive bias; overactive neurons receive negative bias. The updated bias affects input on the following simulation step.

This is a simple homeostatic intrinsic-plasticity approximation. It is not synaptic scaling and is not fitted to a particular biological cell type. The controller is disabled by default, preserving the original simulation behavior. Firing-rate estimates are exposed for experiment telemetry.

## Structural changes

The core can append neurons and synapses at runtime. Neuron IDs are stable, sequential indices and are not reused. Synapse deletion, pruning, connection rewiring, and automatic neuron growth are not implemented yet. They require explicit policies, deterministic behavior, resource budgets, and task-level validation before being added.

## Planned experiments

- Compare fixed weights, ordinary STDP, and reward-modulated STDP on simple associative tasks.
- Measure firing-rate stability with homeostasis disabled and enabled; sweep target rate, estimator time constant, adaptation rate, and bias limits.
- Test delayed reward, causal versus anti-causal spike timing, reversal learning, and retention.
- Add structural plasticity only after a fixed-size network learns a simple association.
- Make growth proposals occur at simulation boundaries, with deterministic seeds and hard neuron/synapse budgets.
- Evaluate whether growth improves held-out performance, learning efficiency, and retention over a matched fixed-topology network.
- Add persistence only after neuron, synapse, and learning state have stable serialization formats.

## Non-goals for the current milestone

- No imported fruit-fly connectome data yet.
- No GPU dependency, external ML framework, or mandatory parallel runtime.
- No claim of consciousness, sentience, or human-level language ability.
