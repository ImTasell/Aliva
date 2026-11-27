# Aliva roadmap and parallel development plan

**Project:** Aliva  
**Language:** C++20  
**Status:** Early research prototype  
**Last updated:** 2026-11-26

This document is the shared planning contract for contributors and coding agents. Read it together with architecture.md and research.md before starting a branch.

## Project objective

Aliva is an experimental, connectome-inspired artificial brain. The long-term research goal is a neural system that can learn from experience, alter its own synaptic structure, and eventually learn symbol sequences and language.

Aliva is **not currently a copied fly brain, a language model, or evidence of artificial consciousness**. Biological connectomes and learning circuits are sources of design ideas. Every capability must be demonstrated by a controlled experiment.

## Current state

### Implemented

- C++20 and CMake project.
- Dependency-free leaky integrate-and-fire (LIF) neuron model.
- Sparse outgoing synapse storage and synchronous simulation steps.
- One-step delayed synaptic event delivery.
- Runtime neuron and synapse creation.
- Fixed synapses via Brain::connect().
- Opt-in plastic synapses via Brain::connect_plastic().
- Pair-based pre/post timing traces, decaying eligibility traces, and signed reward modulation.
- Bounded synaptic weight magnitudes and fixed excitatory/inhibitory polarity.
- A small executable demonstration and CI build/test workflow.
- Research notes in docs/research.md.

The learning-rule tests validate timing signs, delayed reward, fixed-weight behavior, inhibitory polarity, and invalid input handling. **They do not yet demonstrate that Aliva has learned a useful task.**

### Not implemented yet

- A reproducible task/environment and training loop.
- Controlled baseline comparisons and experiment metrics.
- A validated associative-memory behavior.
- Ordinary STDP as an explicit comparator.
- Synaptic delays/receptor models beyond the current simple voltage-kick approximation.
- FlyWire/connectome data import.
- Structural synapse formation, rewiring, neuron growth, or pruning.
- Homeostatic activity regulation.
- Persistent brain checkpoints or memory consolidation.
- Symbol, sequence, or language learning.

## Roadmap at a glance

| Milestone | Goal | Status |
|---|---|---|
| M0 — Neural foundation | Stable LIF simulator, sparse edges, tests, CI | Complete |
| M1 — Associative learning | Learn two cue-to-response associations from feedback | Next |
| M2 — Learning-rule validation | Compare fixed weights, STDP, reward-STDP; test stability and sensitivity | In progress / parallel review |
| M3 — Biological circuit grounding | Import a small, documented mushroom-body-inspired FlyWire subgraph | Planned |
| M4 — Structural development | Bounded synapse growth/rewiring and artificial neuron creation/pruning | Planned |
| M5 — Memory and persistence | Save/load complete neural state and test retention/consolidation | Planned |
| M6 — Symbol and sequence learning | Learn symbols, associations, short sequences, next-symbol prediction | Planned |
| M7 — Language experiments | Small vocabulary and grounded language-generation experiments | Research goal |
| M8 — Scaling | Profile, optimize data layout, then consider SIMD/parallelism/GPU | Later |

---

## M1 — First actual learning: associative conditioning

**Priority: highest. Keep the topology fixed for this milestone.**

Build a small experiment where Aliva receives one of two input cues and chooses one of two output responses. The environment supplies feedback after the response. The cue-to-response mapping must not be wired directly into the network by hand.

### Experiment design

- Inputs: cue A and cue B, represented by distinct sensory input patterns.
- Outputs: response 0 and response 1, represented by output neurons.
- Network: small input, sparse hidden, and output populations. A mushroom-body-inspired sparse hidden layer is acceptable; do not claim this is yet a biological mushroom body.
- Trial: present cue, let the network settle for a defined number of simulation steps, decode an output, then provide a signed reward.
- Training: randomize trial order and use fixed, recorded random seeds.
- Evaluation: use held-out trial sequences and do not update weights during evaluation.

### Required comparisons

1. **Fixed baseline:** identical topology and initial weights, no learning.
2. **Reward-modulated STDP:** task-dependent reward signal.
3. **Control:** shuffled or unrelated reward feedback, to test whether improvement depends on meaningful feedback.
4. **Optional ordinary-STDP comparator:** add an explicit unmodulated pair-based rule rather than quietly treating it as the same thing as task reward.

### Metrics to record

- Choice accuracy by training episode and on held-out trials.
- Trials required to reach a chosen accuracy threshold.
- Reversal-learning performance after swapping cue/response contingencies.
- Retention after a no-training interval.
- Neuron count, synapse count, spike count, firing rate, and weight distribution.
- Random seed and every task/simulation parameter.

### Acceptance criteria

- The same seed and parameters reproduce the same run.
- The learning-enabled network reliably exceeds chance on held-out trials across multiple seeds.
- Fixed and shuffled-reward controls do not show the same improvement.
- Reversal learning demonstrates adaptation to the new mapping rather than memorization of the first mapping.
- No NaNs, unbounded weights, or runaway firing.
- Results are saved in a machine-readable format and summarized in a short experiment report.

Do not pick a single successful seed and call the task solved. Report distributions across seeds.

## M2 — Validate and characterize learning

Run in parallel with M1 as a code audit / test-design activity. Before using the learning rule as scientific evidence, verify:

- Exact sign convention for pre-before-post and post-before-pre pairing.
- Whether reward is an instantaneous teaching event or sustained signal, and how repeated reward updates are intended to accumulate.
- Eligibility decay as a function of the simulation time step.
- Weight clipping, polarity preservation, and behavior near zero weight.
- Behavior with simultaneous spikes, repeated spikes, sparse and dense activity.
- Sensitivity to timestep, trace time constants, learning rate, reward magnitude, and weight bounds.
- Numerical stability and deterministic replay.

The current implementation is an intentionally simple three-factor model. It should be treated as an experimental candidate, not a settled reproduction of biological mushroom-body plasticity.

## M3 — Ground a small circuit in FlyWire

After the synthetic associative task works, select a small, relevant mushroom-body circuit from a versioned FlyWire dataset.

Deliverables:
- Document the dataset name/version, download endpoint, provenance, license/attribution, and extraction procedure.
- Choose a small subgraph with known cell types and relevant PN/KC/MBON/DAN relationships.
- Document missing/uncertain annotations and neurotransmitter assumptions.
- Build a reproducible converter into Aliva's agreed graph format.
- Keep raw large connectome files out of Git; commit only scripts, metadata, and tiny fixtures where licensing permits.
- Compare the imported subgraph against a synthetic circuit on the same task.

Do not download or import an entire whole-brain dataset as the first connectome task.

## M4 — Structural development

Only start after M1 has a strong fixed-topology baseline.

Implement these as separate, testable mechanisms:

1. **Synaptic structural plasticity:** propose and remove connections.
2. **Neuron creation:** allocate a small group of new artificial neurons.
3. **Integration:** give new neurons constrained candidate inputs/outputs and allow learning to select useful connections.
4. **Pruning:** remove persistently unused or unhelpful connections/neurons under explicit rules.
5. **Homeostasis:** regulate activity and prevent silent or runaway circuits.

Growth must be bounded by per-episode and total neuron/synapse budgets. Make growth proposals deterministic under a recorded seed and apply topology changes at simulation boundaries. Log every growth/pruning event.

Compare fixed topology, rewiring-only, and neuron-growth conditions. The growth system is successful only if it improves held-out performance, sample efficiency, or retention without uncontrolled resource growth or unstable activity.

## M5 — Memory and persistence

- Define a versioned checkpoint format for neuron state, synapse weights, plasticity traces, topology, random generator state, and simulation time.
- Test exact save/load continuation against uninterrupted simulation.
- Separate learned long-term state from transient activity state where the model permits it.
- Evaluate interference, retention, and reversal after checkpoint/restart.

Do not implement persistence before the state model is stable.

## M6–M7 — Symbols, sequences, and language

Progress incrementally:

1. Learn associations between a small number of discrete symbols.
2. Learn short sequences and order-sensitive patterns.
3. Predict the next symbol on held-out sequences.
4. Test generalization to new combinations and forgetting/interference.
5. Investigate a small grounded vocabulary and response generation.
6. Compare with simple n-gram and fixed-size recurrent/SNN baselines.

Language is a long-term research goal, not a feature that follows automatically from adding neurons. Record the data source and whether feedback is supervised, self-supervised, or reward-based. Do not use an external language model to generate Aliva's answers while claiming that Aliva learned to talk herself.

## M8 — Performance and scaling

Correctness and task performance come before optimization. Profile realistic workloads, then consider:
- contiguous neuron state arrays and cache-friendly sparse edge storage;
- event queues and batched updates;
- SIMD paths compatible with the target CPU;
- optional multithreading after deterministic update semantics are defined;
- GPU acceleration only if benchmarks justify its complexity.

Keep a portable scalar implementation as the correctness reference.

---

## Parallel branch plan

Coding agents can work in parallel when file ownership is explicit. The following tasks are designed to minimize merge conflicts and avoid several agents independently changing the simulation core.

### Agent A — associative benchmark

**Branch:** feat/association-benchmark  
**Priority:** Start now.

**Owns:**
- experiments/association/**
- tests/test_association.cpp
- CMakeLists.txt (this agent is the only parallel agent allowed to edit it for this milestone)

**Build:**
- Two-cue/two-response environment and seeded trial schedule.
- Input encoding and output decoding.
- Training/evaluation separation.
- Fixed, reward-STDP, and shuffled-reward experiment modes using the existing public Brain API.
- CSV or JSONL result output and a concise sample result document.

**Must not edit:**
- include/aliva/**
- src/brain.cpp, src/neuron.cpp
- tests/test_brain.cpp
- README.md

If an API limitation is discovered, document the needed interface change and ask the integration owner rather than changing core files on this branch.

### Agent B — FlyWire / mushroom-body data preparation

**Branch:** research/mushroom-body-subgraph

**Owns:**
- docs/connectome-import.md
- data/connectome/README.md
- data/connectome/manifest.example.json
- optional small extraction scripts under tools/connectome/**

**Build:**
- Compare relevant FlyWire dataset versions and choose a small mushroom-body region/subgraph.
- Document exact source/version, file formats, attribution, cell-type labels, edge filtering, and uncertain annotations.
- Define a proposed import schema and a reproducible small-subgraph extraction path.
- Do not commit a huge connectome dump or change the runtime's core graph representation.

### Agent C — independent plasticity audit

**Branch:** audit/stdp-validation

**Owns:**
- docs/reviews/stdp-audit.md
- optional standalone mathematical notes/examples under experiments/plasticity-review/**

**Build:**
- Independently review connect_plastic(), trace updates, eligibility decay, reward application, weight clipping, and polarity behavior.
- Verify the intended update rule with small hand-worked timing examples.
- Identify missing edge cases and propose tests/parameter sweeps.
- Report potential correctness or scientific-modeling issues with file/line references.

**Must not edit production implementation files.** Findings should be reviewed before a separate implementation branch is opened.

### Integration owner

**Owns:**
- include/aliva/**
- src/**
- tests/test_brain.cpp
- docs/architecture.md
- final decisions on public API changes and merge order.

The integration owner reviews the experiment API needs and the plasticity audit, then makes any core changes in one focused branch.

---

## Branch and PR rules for every coding agent

1. Start from the latest main, use one named branch per task, and never commit directly to main.
2. Read docs/architecture.md, docs/research.md, and this roadmap before changing anything.
3. Respect the file ownership lists above. If a task needs another agent's files, stop and report the dependency instead of editing them.
4. Keep each PR focused. No unrelated refactors, broad formatting, new heavyweight dependencies, or generated data dumps.
5. Add tests for behavior changes. Run the exact CMake build and CTest commands before opening the PR.
6. Report: summary, files changed, design decisions, tests run/results, known limitations, and any API changes.
7. Do not claim a feature is validated because it compiles. Include task-level experiment results where relevant.
8. No branch should edit README.md; project documentation belongs in docs/.
9. Merge the associative harness and audit/research work first. Integrate any required core API adjustments after review, then use the harness to establish a fixed-size learning baseline.
10. Structural growth work must wait until the associative-learning baseline is understood.

## Recommended merge order

1. audit/stdp-validation review (documentation-only; merge when reviewed).
2. research/mushroom-body-subgraph (documentation/tooling; merge independently).
3. feat/association-benchmark (code + CMake + tests; run CI).
4. A focused core branch for any learning-rule fixes identified by the audit or benchmark integration.
5. Only after M1 is reproducible: open structural-plasticity implementation work.

Parallelism should increase throughput, not produce competing versions of the brain core. Keep one owner for core semantics and merge experimental modules through explicit APIs.
