# Aliva

**Aliva is an experimental, self-developing artificial brain.** The project explores neural computation inspired by biological connectomes, with an eventual goal of learning increasingly complex tasks, including language.

The first milestone is deliberately modest: a small C++ neural simulation core. It is not yet a language model, a complete biological brain, or a direct port of a fruit-fly connectome. The initial implementation provides the low-level machinery we can build and test before introducing learning and structural development.

## Current capabilities

- Leaky integrate-and-fire (LIF) neuron dynamics.
- Sparse, outgoing synapse lists.
- Synchronous simulation steps with one-step synaptic event delivery.
- Runtime creation of neurons and synapses.
- Input validation and a dependency-free test suite.

## Build

Requirements: a C++20 compiler and CMake 3.20 or newer.

~~~sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
~~~

Run the small propagation demo:

~~~sh
./build/aliva
~~~

## Design direction

Aliva is being developed in layers:

1. **Neural dynamics:** neurons, membrane state, spikes, synapses, and deterministic simulation.
2. **Plasticity:** learning rules that adjust synaptic strengths from activity and feedback.
3. **Structural development:** controlled neuron creation, synapse formation, and pruning.
4. **Memory and tasks:** persistent state and measurable learning environments.
5. **Language experiments:** symbol/sequence learning and, later, text generation.

The order matters. We will evaluate each layer independently instead of assuming that adding neurons automatically creates intelligence.

See the architecture notes in docs/architecture.md for the initial model and its constraints.
