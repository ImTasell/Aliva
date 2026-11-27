# M2 associative-learning comparison results

**Run:** [GitHub Actions paired-seed benchmark #45936491146](https://github.com/ImTasell/Aliva/actions/runs/45936491146)  
**Artifact:** `aliva-association-comparison` (8 seeds per condition; raw JSONL trials and per-seed JSON summaries)  
**Configuration:** default task/network parameters; seeds 1–8; same task RNG stream, cue schedule, topology, and noise for paired conditions. Shuffled reward uses an isolated RNG stream.

These are results from one controlled synthetic task, not evidence of general intelligence or biological equivalence.

## Aggregate results

Values are mean ± sample standard deviation across eight seeds.

| Condition | Held-out accuracy | Retention | Reversal eval | Threshold reached | Mean firing rate |
|---|---:|---:|---:|---:|---:|
| Fixed weights | 0.547 ± 0.159 | 0.516 ± 0.200 | 0.466 ± 0.189 | 4/8 | 25.57 Hz |
| Ordinary STDP | 0.694 ± 0.440 | 0.681 ± 0.457 | 0.313 ± 0.458 | 5/8 | 25.65 Hz |
| Reward-modulated STDP | **0.994 ± 0.018** | **1.000 ± 0.000** | **0.691 ± 0.237** | 7/8 | 25.58 Hz |
| Reward STDP + homeostasis | 0.409 ± 0.219 | 0.372 ± 0.253 | 0.113 ± 0.078 | 3/8 | 11.77 Hz |
| Shuffled reward | 0.594 ± 0.203 | 0.550 ± 0.154 | 0.453 ± 0.171 | 3/8 | 25.55 Hz |

The threshold column is the number of seeds that reached the configured episode-accuracy threshold at least once. Mean trials-to-threshold is calculated only among those successful seeds, so it should not be compared without the reach fraction.

## Held-out accuracy by paired seed

| Seed | Fixed | Ordinary STDP | Reward STDP | Reward + homeostasis | Shuffled reward |
|---:|---:|---:|---:|---:|---:|
| 1 | 0.400 | 0.000 | 1.000 | 0.350 | 0.625 |
| 2 | 0.600 | 1.000 | 1.000 | 0.525 | 0.500 |
| 3 | 0.475 | 1.000 | 1.000 | 0.775 | 0.500 |
| 4 | 0.625 | 0.975 | 0.950 | 0.400 | 0.250 |
| 5 | 0.700 | 1.000 | 1.000 | 0.450 | 0.775 |
| 6 | 0.425 | 0.075 | 1.000 | 0.000 | 0.875 |
| 7 | 0.350 | 0.500 | 1.000 | 0.300 | 0.750 |
| 8 | 0.800 | 1.000 | 1.000 | 0.475 | 0.475 |

## Interpretation

1. **Reward-modulated STDP is the strongest and most reliable condition in this experiment.** It exceeded the fixed baseline on all eight paired seeds; the mean paired improvement was 0.447. It also exceeded shuffled reward on all eight paired seeds, with a mean paired improvement of 0.400. Retention was 1.0 across all seeds, but this is passive retention across an idle interval, not active consolidation.
2. **Ordinary STDP is highly seed-sensitive.** Its mean held-out accuracy was 0.694, but the standard deviation was 0.440. Five seeds were at or above 0.975 while two were near zero and one was 0.5. It has no access to the task's correct/incorrect signal, so high scores can reflect favourable initial cue/read-out structure being reinforced by unsupervised spike correlations. It did not reliably adapt to the reversed mapping (mean reversal accuracy 0.313).
3. **The shuffled-reward control is not yet a strong negative control at n=8.** Its mean was 0.594, with two seeds at 0.775 and 0.875. Random reward can correlate with correctness by chance over finite runs. Increase the seed count before making a stronger claim about the null condition.
4. **The current homeostasis settings are too disruptive for this task.** The controller targets 5 Hz with adaptation rate 0.05 and maximum bias magnitude 1.0 mV/ms. Mean firing rate fell from about 25.6 Hz to 11.8 Hz, while mean absolute bias reached about 0.775 mV/ms. Accuracy fell to 0.409. This shows that this parameterization suppresses the circuit's activity and interferes with the learned read-out; it does not show that homeostasis in general is harmful.
5. **Reversal remains a weakness of reward-modulated STDP.** The mean reversal evaluation was 0.691 with substantial between-seed variation. Acquisition is robust, but reversal is not yet consistently stable.

## Next experiments

- Sweep homeostatic target rates, adaptation rates, and bias limits; record bias saturation and per-population firing rates rather than only one network-wide mean.
- Increase the shuffled-reward and ordinary-STDP comparison to at least 32 seeds if compute budget allows.
- Test overlapping hidden representations to measure how much the current labelled-line pattern separation is carrying the task.
- Keep the fixed-topology network for now. Structural growth should wait until learning, retention, and reversal are understood under these controlled comparisons.

