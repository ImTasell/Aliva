# Aliva

**Aliva is an experimental, self-developing artificial brain.** The project explores neural computation inspired by biological connectomes, with an eventual goal of learning increasingly complex tasks, including language.

The first milestone is deliberately modest: a small C++ neural simulation core. It is not yet a language model, a complete biological brain, or a direct port of a fruit-fly connectome. The initial implementation provides the low-level machinery we can build and test before introducing learning and structural development.

## Current capabilities

- Leaky integrate-and-fire (LIF) neuron dynamics.
- Sparse, outgoing synapse lists.
- Synchronous simulation steps with one-step synaptic event delivery.
- Runtime creation of neurons and synapses.
- Fixed synapses and opt-in plastic synapses.
- Pair-based STDP eligibility traces with delayed reward modulation.
- Fixed excitatory/inhibitory polarity and bounded synaptic weight magnitude.
- Dependency-free test suite.

## Build

Requirements: a C++20 compiler and CMake 3.20 or newer.

~~~sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
~~~

Run the reward-modulated STDP demonstration:

~~~sh
./build/aliva
~~~

## Design direction

Aliva is being developed in layers:

1. **Neural dynamics:** neurons, membrane state, spikes, synapses, and deterministic simulation.
2. **Synaptic learning:** timing-dependent eligibility and reward-modulated weight updates.
3. **Associative memory:** small mushroom-body-inspired learning experiments.
4. **Structural development:** controlled neuron creation, connection rewiring, and pruning.
5. **Memory and language experiments:** persistent learned state, symbol/sequence learning, and eventually text generation.

The order matters. We will evaluate each layer independently instead of assuming that adding neurons automatically creates intelligence.

See [docs/architecture.md](docs/architecture.md) for the implementation model and [docs/research.md](docs/research.md) for the literature review and experiment plan.
