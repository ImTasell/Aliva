# M3 learning validation

## Purpose

This validation checks whether the promising eight-seed homeostasis results hold
up across a larger paired seed set before Aliva's learning architecture is
expanded. It does not change the neural model or claim biological fidelity.

## Conditions

The workflow runs the same consecutive seed range for every condition:

| Condition | Mode | Homeostasis target | Adaptation rate | Bias cap |
|---|---|---:|---:|---:|
| Reward-STDP baseline | `reward` | disabled | — | — |
| Shuffled-reward control | `shuffled` | disabled | — | — |
| Homeostasis cap 0.15 | `reward-homeostasis` | 5 Hz | 0.005 | 0.15 mV/ms |
| Homeostasis cap 0.25 | `reward-homeostasis` | 5 Hz | 0.005 | 0.25 mV/ms |

The default is 32 seeds beginning at seed 1. The seed range can be changed when
manually dispatching the workflow. The same seed is used across all conditions,
preserving the paired input-pattern and noise schedules. The shuffled condition
uses its separate reward RNG stream.

## Telemetry

Each per-seed summary records the final exponentially filtered firing-rate
estimate for every neuron, aggregated separately for input, hidden, and output
populations. For each population it reports:

- neuron count;
- mean firing rate and within-population standard deviation;
- mean absolute homeostatic bias;
- fraction of neurons at the configured bias limit.

The network-wide bias-saturation fraction is the number of neurons whose
absolute bias is within a relative tolerance of the configured limit, divided
by the total neuron count. This is a saturation diagnostic, not evidence that
the target firing rate has been reached.

These are end-of-run filtered estimates sampled after reversal evaluation.
They are not phase-specific firing-rate averages. The separate whole-run
spike-rate field already present in the summary is retained for continuity.

## Outputs

The workflow uploads the raw per-trial JSONL files, one summary JSON per seed,
and `comparison.csv` as the `aliva-m3-validation` artifact. The CSV includes
condition means and sample standard deviations across seeds, including the new
population telemetry.

## Interpretation checklist

1. Compare reward-STDP and each homeostasis cap using paired per-seed
   differences, not just condition means.
2. Check acquisition, passive retention, and reversal accuracy together.
3. Inspect whether homeostasis changes firing unequally across input, hidden,
   and output populations.
4. Treat a high bias-saturation fraction as a warning that the controller is
   spending time at its actuator limit.
5. Revisit the shuffled-reward control at the larger sample size; a strong
   negative control should not consistently approach the informative reward
   condition.
6. Do not select a global default or add structural plasticity from this
   benchmark alone. Confirm stability and then proceed to multi-association,
   interference, and forgetting tests.

The benchmark is a small synthetic labelled-line circuit. Its current strong
pattern separation makes it useful for comparing learning rules, but it does
not yet establish that Aliva can learn multiple overlapping associations.
