# Associative-learning experiment (M1)

**Status:** reference implementation for milestone M1 (associative learning)
**Branch:** `feat/association-benchmark-main`
**Scope:** experiment harness only. This document describes the task, the code,
how to run it, and how to interpret the results. It does **not** modify the
neural core (`include/aliva/**`, `src/**`) or the existing core tests.

---

## 1. Objective

Demonstrate that Aliva's existing reward-modulated STDP rule can learn a simple
two-cue / two-response association from feedback, and that the learned behaviour
is not an artefact of the harness:

- the **learning-enabled** network exceeds chance on held-out trials;
- a **fixed-weight** baseline and a **shuffled-reward** control do not;
- the network **adapts** when the cue→response contingency is reversed;
- the learned association is **retained** across a no-training interval.

The cue→response mapping is **never** wired into the network. Only the
environment's reward rule knows which response is correct for each cue.

> This is a controlled demonstration of a candidate learning rule, not evidence
> of biological mushroom-body function or general intelligence.

---

## 2. Task design

### 2.1 Populations

| Population | Size (default) | Role |
|---|---|---|
| Input (sensory) | 24 | two disjoint cue patterns of 6 neurons each |
| Hidden (Kenyon-cell-like) | 160 | sparse, fixed input→hidden connectivity |
| Output (read-out) | 2 | response 0 and response 1 |

Total: 186 neurons. Connectivity is fixed in topology for this milestone.

### 2.2 Cues (seeded input patterns)

`make_cue_patterns()` draws two **disjoint** random subsets of the input neurons
from the run's seeded generator: cue **A** drives one subset, cue **B** drives a
different, non-overlapping subset. Disjoint patterns maximise the sensory
separation available to the network. The same seed always reproduces the same
patterns.

### 2.3 Hidden layer (labelled-line pattern separation)

Each hidden neuron is assigned to **one cue channel** and samples its fixed
inputs only from that cue's pattern. The two cues therefore activate disjoint
Kenyon-cell-like populations. This is a strong, explicit form of pattern
separation (a "labelled line" per sensory channel); it is a *design choice* that
keeps the two associations from interfering, and is discussed as a limitation in
§9. Each hidden neuron projects to a **single** output neuron (divergent
read-out), mirroring the mushroom-body readout motif.

### 2.4 Trial lifecycle

One trial (`Experiment::run_trial`):

1. **Cue presentation + read-out window** (`settle_steps`, default 50 ms): the
   cue's input neurons receive external current; zero-mean Gaussian background
   noise is added to every neuron for exploration. Output spikes and first-spike
   steps are recorded.
2. **Decode** the response from the two output neurons (§3).
3. **Reward step** (`reward_steps`, default 1): a signed modulatory signal is
   passed to `Brain::step`. This is where plasticity converts eligibility into
   weight changes. In evaluation the reward is exactly `0.0`, so no weight can
   change.
4. **Inter-trial interval** (`iti_steps`, default 60 ms): no input, no reward;
   the membrane state settles.

### 2.5 Output decoding (deterministic)

`decode_response()` is a pure function of the read-out:

1. **First-spike latency** — the output that spikes earliest wins (earlier =
   stronger net drive). This is a graded, non-saturating code.
2. Ties → **total spike count**.
3. Ties → **final membrane potential** (higher wins).
4. Exact tie → response 0.

First-spike latency is used because raw spike counts saturate at the refractory
limit and lose the drive difference that learning must act on.

---

## 3. Experimental modes

`--mode` selects the condition. Topology and initial weights are identical
across modes for a given seed; the learning rule and optional homeostatic
controller are the experimental variables.

| Mode | Hidden→output synapses | Reward signal |
|---|---|---|
| `fixed` | fixed (`connect`) | logged (correct/incorrect) but cannot change weights |
| `stdp` | ordinary STDP (`connect_stdp`) | none; timing updates weights directly |
| `reward` | plastic (`connect_plastic`) | `+1` if the decoded response is correct, `−1` otherwise |
| `reward-homeostasis` | reward-modulated STDP + intrinsic-bias homeostasis | task reward; homeostasis active during training only |
| `shuffled` | plastic (`connect_plastic`) | exactly-balanced, seeded-shuffled `±1`, independent of the response |

Ordinary STDP is now available as an M2 comparison condition. It receives no task reward; the experiment pauses all synaptic learning during held-out evaluation so evaluation cannot change its weights. Homeostasis is likewise paused during evaluation and retention.

The **shuffled** control keeps the reward statistics of the learning condition
(half `+1`, half `−1`) but destroys the contingency, testing whether improvement
depends on *meaningful* feedback.

### Training vs. evaluation separation

Evaluation phases (`eval`, `retention`, `reversal_eval`) pass `reward = 0.0`
and explicitly disable synaptic learning. This pauses both direct ordinary-STDP
updates and reward-modulated weight updates, so held-out evaluation cannot change
synaptic weights. Homeostatic bias adaptation is also paused during evaluation
and retention; firing-rate telemetry continues updating.

---

## 4. Schedule

`Experiment::run()` executes, in order:

1. **Acquisition training** — `train_episodes` × `trials_per_episode` trials,
   random cue order, reward applied.
2. **Held-out evaluation** — `eval_trials` trials, no weight updates.
3. **Retention** — `retention_idle_steps` idle steps (no input, no reward),
   then `retention_trials` evaluation trials.
4. **Reversal training** — the contingency is swapped (A→1, B→0) and the
   network is trained again (`reversal_episodes` × `reversal_trials_per_episode`).
5. **Reversal evaluation** — `reversal_eval_trials` held-out trials under the new
   contingency.

All randomness (patterns, connectivity, trial order, exploration noise, shuffled
reward) flows from the single seeded `std::mt19937_64`, so a run is fully
reproducible from its seed.

---

## 5. Metrics

Per-trial records (§7) plus an aggregate summary:

- `episode_accuracy` — training accuracy per episode (acquisition curve).
- `train_accuracy`, `eval_accuracy` — pooled training / held-out accuracy.
- `trials_to_threshold` — training trials to first reach `accuracy_threshold`.
- `reversal_episode_accuracy`, `reversal_eval_accuracy` — adaptation after swap.
- `retention_accuracy` — accuracy after the no-training interval.
- `total_spikes`, `mean_firing_rate_hz`, `mean_abs_homeostatic_bias_mV_per_ms`, `neuron_count`, `synapse_count`.
- `output_weights` — min/max/mean/mean-abs of the hidden→output weights.

---

## 6. Command line

Build and test (from the repository root):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/aliva_association --help
```

Run the three conditions for one seed:

```sh
./build/aliva_association --mode reward   --seed 1 --out /tmp/reward
./build/aliva_association --mode stdp     --seed 1 --out /tmp/stdp
./build/aliva_association --mode reward-homeostasis --seed 1 --out /tmp/reward-homeostasis
./build/aliva_association --mode fixed    --seed 1 --out /tmp/fixed
./build/aliva_association --mode shuffled --seed 1 --out /tmp/shuffled
```

For a paired M2 comparison, run every mode with the same seed range (for example `--seeds 8 --seed 1`). The same seed reproduces cue patterns, connectivity, cue order, and noise across modes. Shuffled reward uses a separate RNG stream so its control bookkeeping cannot perturb the shared task randomness.

Run several seeds and emit JSONL:

```sh
./build/aliva_association --mode reward --seeds 5 --format jsonl --out /tmp/reward
```

Useful overrides: `--episodes`, `--trials`, `--eval`, `--rev-episodes`,
`--rev-trials`, `--rev-eval`, `--retention`, `--idle`, and the advanced tuning
flags (`--lr`, `--pot`, `--dep`, `--elig-tau`, `--maxw`, `--out-weight`,
`--hidden-weight`, `--inh`, `--jitter`, `--noise`, `--settle`). See `--help`.

Each run writes `<prefix>_trials.<csv|jsonl>` and `<prefix>_summary.json`.

---

## 7. Output format

**Per-trial CSV** (`<prefix>_trials.csv`):

```
mode,seed,trial,episode,phase,cue,correct_response,selected_response,reward,correct,output0_spikes,output1_spikes,total_spikes,sim_time_ms
```

**Per-trial JSONL** (`<prefix>_trials.jsonl`): one JSON object per trial with the
same fields.

**Summary** (`<prefix>_summary.json`): the aggregate metrics of §5.

Example rows (reward, seed 1):

```
reward_stdp,1,0,0,train,A,0,0,1,1,4,4,1264,0
reward_stdp,1,1,0,train,A,0,0,1,1,4,4,1304,111
...
```

---

## 8. Results

Default configuration, 8 fixed seeds (1–8). Accuracy is the fraction of correct
held-out trials; reversal is the held-out accuracy after the contingency swap.

| Mode | Held-out accuracy (mean) | Per-seed held-out accuracy |
|---|---|---|
| reward (reward-modulated STDP) | **0.99** | 1.0, 1.0, 1.0, 0.95, 1.0, 1.0, 1.0, 1.0 |
| fixed (baseline) | 0.55 | 0.40, 0.60, 0.475, 0.625, 0.70, 0.425, 0.35, 0.80 |
| shuffled (control) | 0.36 | 0.45, 0.70, 0.175, 0.10, 0.35, 0.45, 0.525, 0.125 |

Reward-modulated STDP acquires the association reliably (held-out ≈ 0.99) and
retains it perfectly (retention = 1.0 on every seed). The fixed baseline sits at
chance (mean 0.55) and the shuffled control is at or below chance (mean 0.36):
neither shows the improvement of the learning condition, so the gain depends on
meaningful feedback.

**Reversal** (reward condition, per-seed held-out accuracy after the swap):
0.625, 0.60, 0.85, 1.0, 0.625, 0.275, 0.55, 0.425 (mean ≈ 0.62). The network
clearly *unlearns* the old mapping (early reversal episodes drop to ≈ 0.0–0.2)
and then reassigns the associations (later episodes rise well above the old-mapping
baseline). Full, stable symmetric reversal is **not** achieved on every seed; see
§9.

Representative single-seed summary (`reward`, seed 1):

```json
{
  "mode": "reward_stdp", "seed": 1,
  "train_accuracy": 0.894, "eval_accuracy": 1, "trials_to_threshold": 40,
  "reversal_eval_accuracy": 0.625, "retention_accuracy": 1,
  "neuron_count": 186, "synapse_count": 619,
  "episode_accuracy": [0.5, 0.85, 0.9, 1, 0.9, 1, 1, 1]
}
```

Result files are generated locally by the commands above; benchmark output artifacts are not checked into the repository.

---

## 9. Limitations and honest caveats

1. **Global-reward credit assignment.** `Brain::step` applies one scalar reward
   to *all* eligible synapses. With two competing read-out neurons this makes
   credit assignment hard: the winner-take-all inhibition plus exploration noise
   let the correct pathway be reinforced, but the *losing* correct pathway is not
   directly targeted. This is the main reason reversal is variable.
2. **Reversal is not fully reliable.** Acquisition and retention are robust, but
   complete symmetric reversal depends on the seed (mean ≈ 0.62, range
   0.275–1.0). The network adapts (it does not merely memorise the first
   mapping), yet a strong learned asymmetry plus a hard weight bound can leave a
   residual, frozen association. Stronger exploration during reversal was tested
   and *degraded* reversal (it destroys the learned state without a stable
   re-learning signal), so it is not used.
3. **Labelled-line hidden layer.** Each hidden neuron samples a single cue
   channel. This guarantees pattern separation and is what makes learning
   reliable, but it is a strong architectural assumption. A fully overlapping
   (non-labelled-line) hidden layer was tried and produced unreliable, seed-
   dependent learning; that is recorded here rather than hidden.
4. **Fixed baseline bias.** With a fixed random network the response is a fixed
   function of the cue, so per-seed fixed accuracy is quantised (it lands near
   0.4/0.6/0.8 rather than exactly 0.5); the mean over seeds is at chance.
5. **Shuffled-control variance.** The shuffled reward is exactly balanced and
   uncorrelated in expectation, but a finite run can by chance correlate with
   correctness and produce weak spurious learning on some seeds (mean 0.36, max
   0.70). It never matches the learning condition.
6. **No consolidation / persistence.** The core now has optional homeostatic intrinsic-bias regulation, but this M1 experiment leaves it disabled. There is no weight decay, checkpointing, or active consolidation. “Retention” here only verifies that weights persist unchanged across a no-training interval.
7. **Model parameters are not biological.** Weights, thresholds, time constants,
   learning rate, and the weight bound are simulation units chosen for this task,
   not fitted to any cell type. The weight bound (`max_abs_weight_mV = 0.3`) is
   deliberately tight to keep the learned asymmetry bounded.
8. **Reproducibility scope.** Runs are bit-reproducible on the same standard
   library / platform. `std::uniform_*_distribution` and `std::normal_distribution`
   are not guaranteed identical across different standard-library
   implementations.

---

## 10. Relationship to the roadmap

This harness is the M1 deliverable. It uses only the public `Brain` API
(`add_neuron`, `connect`, `connect_plastic`, `step`, `synapses_from`,
`neuron_count`, `synapse_count`, `simulation_time_ms`). No core files were
changed. If a capability is missing (e.g. targeted per-synapse reward,
homeostasis, checkpointing), it is listed above rather than worked around in the
core.

### Owned paths

- `experiments/association/**`
- `tests/test_association.cpp`
- `CMakeLists.txt`

### Not modified

- `include/aliva/**`, `src/**`, `tests/test_brain.cpp`,
  `docs/architecture.md`, `docs/research.md`, `README.md`.

### Tests

`tests/test_association.cpp` covers: seeded/distinct cue patterns, deterministic
decoding, network size, fixed weights never changing, reward weights changing,
zero reward during evaluation, same-seed reproducibility, different-seed
variation, above-chance acquisition, controls not learning, reversal adaptation,
summary consistency, CSV/JSONL export, and configuration validation.

## 11. Automated paired-seed comparison

The repository includes a manual GitHub Actions workflow named **Associative learning comparison**. Open the Actions tab, select that workflow, choose **Run workflow**, and set the seed count (default 8) and first seed (default 1). It runs `fixed`, `stdp`, `reward`, `reward-homeostasis`, and `shuffled` with the same seed range, then uploads an artifact named `aliva-association-comparison`.

The artifact contains per-seed JSONL trial records, per-seed JSON summaries, and `comparison.csv` with mean and standard deviation for held-out accuracy, retention, reversal, firing rate, and related metrics. This keeps the benchmark off the normal build/test path while making full comparisons reproducible.
