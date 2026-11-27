# Aliva research brief: connectome-inspired learning and self-development

**Status:** Initial literature review, 26 November 2026  
**Purpose:** Make explicit, evidence-based choices for the first learning and structural-plasticity experiments.

## Executive recommendation

Aliva should be developed as a **connectome-inspired, plastic spiking network**, not presented as a literal copy of a complete fly brain. The most useful biological starting point is the Drosophila mushroom body, a well-studied associative-learning circuit. A practical first experiment is a small mushroom-body-inspired network that learns cue–outcome associations through reward-modulated plasticity. Once that works reliably, add bounded structural plasticity and test whether growth improves learning.

Do not begin by importing the entire fly connectome or attempting conversational language. First establish a reproducible learning result with a small network and clear baselines.

## 1. What the fly connectome gives us

The FlyWire whole-brain reconstruction describes an adult female Drosophila melanogaster brain. The flagship 2024 paper reports 139,255 reconstructed neurons and approximately 50 million chemical synapses, alongside cell-type, region, and neurotransmitter annotations.

The published computational model based on this connectome used leaky integrate-and-fire neurons and connectivity-derived weights to model sensory-to-motor processing. It predicted activity related to feeding and grooming and generated experimentally testable hypotheses. It is an important feasibility reference for connectome-based simulation.

**Important limitation:** a connectome is an anatomical snapshot, not a complete learning algorithm. The computational model did not establish that the fixed network learns new associations, grows neurons, or acquires language. The published model also simplified neurons to excitatory or inhibitory spiking units and explicitly ignored morphology and receptor dynamics; its authors note that dopamine/serotonin neurons are not well represented by this binary treatment.

The FlyWire network-statistics paper also notes limitations in predicted transmitter identities and missing signaling modalities such as gap junctions and neuropeptide signaling. Therefore Aliva should preserve uncertainty and provenance in imported annotations rather than treating every edge and sign as ground truth.

### Dataset choice

The current Codex dataset page lists:
- **FAFB v783:** female adult fly brain; 139,255 neurons and 3,732,460 neuron-to-neuron connections.
- **BANC v888:** female adult brain and nerve cord; 158,262 neurons and 3,037,361 neuron-to-neuron connections.

The Codex FAQ describes these as versioned static snapshots. The counts above are connection pairs in the selected data release, not a count of individual chemical synapses. This distinction matters: papers may report roughly 50 million synaptic contacts while a thresholded connectome edge list contains far fewer neuron-pair connections.

**Recommendation:** use FAFB v783 initially because it is the well-documented adult female brain used by the core 2024 literature. Extract only a small, annotated subnetwork for experiments. Do not download the full dataset as the first step. Keep the dataset name, version, source file, filtering threshold, and annotation provenance with every imported graph. Re-evaluate BANC when Aliva needs brain-to-nerve-cord pathways.

## 2. The mushroom body is the right learning inspiration

The Drosophila mushroom body is a major associative-learning and memory center. Its well-studied computational motifs include:

- **Projection neurons (PNs):** deliver sensory patterns.
- **Kenyon cells (KCs):** represent inputs sparsely and combinatorially, supporting pattern separation.
- **Mushroom body output neurons (MBONs):** provide learned output/readout pathways.
- **Dopaminergic neurons (DANs):** convey reinforcement-related signals that modulate plasticity in mushroom-body compartments.

Research on the adult mushroom body mapped its learning-related circuitry, including Kenyon-cell inputs to output neurons and dopaminergic modulation. Reviews describe sparse coding, convergence, compartmentalized plasticity, multiple memory traces, and interactions between modules.

This provides a more grounded initial architecture than a fully random network. However, the exact biological learning rule is still debated; the 2024 review of 25 years of mushroom-body models explicitly treats the algorithmic question as open. Aliva should compare candidate learning rules instead of claiming to reproduce the fly's exact mechanism.

### First experimental circuit

Build a small synthetic mushroom-body-inspired circuit first:

1. A small input population encodes two or more distinct cues.
2. A sparse hidden population plays the role of Kenyon cells.
3. A small output population represents alternative learned responses.
4. A modulatory signal represents reward or punishment.

Train it on a simple association task, such as cue A predicting reward and cue B predicting no reward (or punishment). Measure whether the system learns the association, retains it, and updates it when the contingency changes.

Once that experiment is stable, replace the synthetic connectivity with a carefully extracted and annotated FlyWire subgraph.

## 3. Functional plasticity: learning without changing network size

Start with synaptic plasticity before adding neuron growth.

### Candidate: reward-modulated STDP with eligibility traces

Spike-timing-dependent plasticity (STDP) changes a synapse based on the relative timing of pre- and postsynaptic activity. For reinforcement learning, a useful extension is a **three-factor rule**:

1. Presynaptic activity.
2. Postsynaptic activity.
3. A modulatory reward or reward-prediction signal.

The pre/post activity establishes a temporary eligibility trace at a synapse. A later reward signal can convert that trace into a lasting weight change. Florian (2007) demonstrated reward-modulated STDP rules in integrate-and-fire network simulations, including XOR and target firing-rate tasks.

A useful conceptual rule is:

~~~text
weight_change = learning_rate * modulatory_signal * eligibility_trace
~~~

This is a candidate learning mechanism, not a claim that the fly uses exactly this equation. Compare at least:
- ordinary pair-based STDP;
- reward-modulated STDP with an eligibility trace;
- a simple supervised or reward-only baseline for comparison.

Do not interpret reward as a magic scalar that solves credit assignment. The timing, sign, baseline/expected reward, and decay of eligibility traces all need controlled experiments.

## 4. Structural plasticity: changing the network itself

Keep these mechanisms distinct:

- **Weight plasticity:** existing synapses become stronger or weaker.
- **Synaptic structural plasticity:** connections form, disappear, or rewire.
- **Neuron growth:** Aliva creates additional artificial neurons and integrates them into the network.

Research distinguishes Hebbian structural changes, which can reinforce active pathways, from homeostatic structural changes, which help regulate network activity and stability. Computational work has demonstrated STDP-driven rewiring, but these results do not imply that unrestricted neuron growth produces intelligence.

### Proposed bounded growth controller

Treat neuron creation as an experimental policy with explicit resource limits:

1. Track learning performance and prediction error over a window, not a single failed example.
2. Track local firing rates and activity saturation.
3. Only propose growth when the task has plateaued and the relevant circuit has evidence of insufficient capacity.
4. Add a small neuron group with weak, constrained candidate connections.
5. Allow plasticity to select useful connections.
6. Remove weak/redundant connections and prune neurons only under explicit criteria.
7. Enforce maximum neuron, synapse, and growth-per-interval budgets.

Use homeostatic regulation to prevent runaway excitation or silent networks. Evaluate growth by whether it improves held-out task performance and retention relative to an otherwise identical network with growth disabled.

**Scientific wording:** this is software neurogenesis inspired by biological structural plasticity. It is not a claim that the artificial process exactly reproduces biological adult neurogenesis.

## 5. Language: possible research direction, not the next milestone

Spiking language research exists:
- **SpikeLM (ICML 2024)** explores fully spike-driven mechanisms for discriminative and generative language tasks and discusses the information limitations of simple binary spikes.
- **SpikeLLM (ICLR 2025)** studies spiking mechanisms in large language-model-scale systems.

These papers show that spiking mechanisms can be investigated in language-model settings. They do **not** show that a tiny connectome-inspired network can learn open-ended conversation from scratch, nor that adding neurons alone will yield language ability. Large-scale spiking language work is not equivalent to a small self-growing biological-style brain.

Recommended progression:
1. Learn associations between a few discrete symbols.
2. Learn short symbol sequences and next-symbol prediction.
3. Learn a small vocabulary grounded in simple tasks and feedback.
4. Evaluate generalization to unseen combinations and retention after interruptions.
5. Only then investigate larger text corpora and richer language generation.

Use clear held-out datasets and compare against simple n-gram, recurrent, and fixed-size SNN baselines. Keep the source of training data and whether feedback is supervised, self-supervised, or reward-based explicit.

## 6. Engineering implications for the C++ core

The current core is a deliberately small LIF demonstration. It has sparse outgoing synapse lists and one-step delayed signed voltage kicks. It does not yet implement plasticity, receptor kinetics, biological neurotransmitter classes, growth rules, connectome import, or persistence.

Before adding learning, the simulator should expose enough state to implement and test learning rules deterministically.

Suggested next layers:

### Neural dynamics
- Keep the initial LIF model as a baseline.
- Introduce explicit excitatory/inhibitory and modulatory synapse semantics rather than relying only on a signed scalar.
- Add a configurable synaptic response model and explicit transmission delay.
- Validate time-step behavior and deterministic event delivery.

### Plasticity
- Store pre/post traces and eligibility state per plastic synapse.
- Add reward/modulatory input to the simulation API.
- Bound weights and expose learning-rule parameters.
- Add tests proving the direction, timing, decay, and reward dependence of weight changes.

### Structural development
- Add stable neuron IDs and safe edge insertion/removal.
- Separate growth proposals from application of topology changes (apply at simulation boundaries).
- Add per-network growth budgets, event logs, and deterministic random seeds.
- Make growth decisions inspectable and replayable.

### Experiments and persistence
- Separate simulation, task/environment, training loop, and evaluation.
- Record seeds, parameters, network size, spike counts, weights, reward, and task metrics.
- Add checkpointing only after the model state is clearly specified.
- Benchmark after correctness; avoid premature threading or GPU dependencies.

### Existing C++ research software to study

- **NEST:** a C++ simulation kernel for spiking networks, with models for synaptic plasticity and structural plasticity. Useful as a reference for simulation semantics and validation.
- **Arbor:** a C++17 neural simulation library supporting LIF and detailed multicompartment cells, with examples for STDP and structural plasticity. Useful for understanding performance-oriented simulation design.

Aliva does not need to adopt either dependency now. Use their documentation and examples as references, then benchmark only if the custom engine needs it.

## 7. Proposed experiments and acceptance criteria

### Experiment A — associative learning
- Two or more input cues.
- One reward/punishment modulator.
- Sparse hidden/Kenyon-like population.
- Output choice between responses.
- Compare no-learning, STDP, and reward-modulated STDP.
- Record acquisition curve, held-out cue discrimination, reversal learning, and retention.

### Experiment B — structural adaptation
Run the same task with:
- fixed topology;
- weight plasticity only;
- weight plasticity plus synapse rewiring;
- weight plasticity plus bounded neuron growth and pruning.

Evaluate accuracy, sample efficiency, retention, network size, firing-rate stability, and compute cost. Growth is useful only if it provides a measurable benefit without destabilizing the system or growing without bound.

### Experiment C — sequence learning
After associative learning works, give Aliva short symbol sequences and test next-symbol prediction, sequence length generalization, and forgetting/interference. Keep the task tiny at first.

## 8. Key references

1. Dorkenwald et al. (2024), **Neuronal wiring diagram of an adult brain**, Nature. https://doi.org/10.1038/s41586-024-07558-y
2. Shiu et al. (2024), **A Drosophila computational brain model reveals sensorimotor processing**, Nature. https://doi.org/10.1038/s41586-024-07763-9
3. Schlegel et al. (2024), **Whole-brain annotation and multi-connectome cell typing of Drosophila**, Nature. https://doi.org/10.1038/s41586-024-07686-5
4. Modi, Shuai & Turner (2020), **The Drosophila Mushroom Body: From Architecture to Algorithm in a Learning Circuit**, Annual Review of Neuroscience. https://doi.org/10.1146/annurev-neuro-080317-062133
5. Eichler et al. (2017), **The complete connectome of a learning and memory centre in an insect brain**, Nature. https://doi.org/10.1038/nature23455
6. Webb (2024), **Beyond prediction error: 25 years of modeling the associations formed in the insect mushroom body**, Learning & Memory. https://doi.org/10.1101/lm.053824.123
7. Florian (2007), **Reinforcement learning through modulation of spike-timing-dependent synaptic plasticity**, Neural Computation. https://doi.org/10.1162/neco.2007.19.6.1468
8. Butz, Wörgötter & van Ooyen (2009), **Activity-dependent structural plasticity**, Brain Research Reviews. https://doi.org/10.1016/j.brainresrev.2008.12.023
9. Lobov et al. (2023), **STDP-Driven Rewiring in Spiking Neural Networks under Stimulus-Induced and Spontaneous Activity**, Biomimetics. https://doi.org/10.3390/biomimetics8030320
10. Xing et al. (2024), **SpikeLM: Towards General Spike-Driven Language Modeling via Elastic Bi-Spiking Mechanisms**, ICML/PMLR. https://proceedings.mlr.press/v235/xing24d.html
11. Xing et al. (2025), **SpikeLLM: Scaling up Spiking Neural Network to Large Language Models via Saliency-based Spiking**, ICLR. https://openreview.net/forum?id=9i7JdpU3eq
12. FlyWire Codex dataset and version FAQ: https://codex.flywire.ai/faq?dataset=fafb
13. NEST documentation: https://nest-simulator.org/
14. Arbor documentation: https://docs.arbor-sim.org/en/latest/

---

## Bottom line

The next milestone should be **reward-modulated synaptic learning in a small mushroom-body-inspired network**, with an evaluation task and baselines. Do not implement automatic neuron growth until Aliva can learn a simple association without growth. Then we will have a meaningful baseline to test whether self-development actually helps.
