# Homeostasis parameter sweep results

**Run:** [GitHub Actions homeostasis sweep #45936942668](https://github.com/ImTasell/Aliva/actions/runs/45936942668)  
**Artifact:** `aliva-homeostasis-sweep`  
**Design:** eight paired seeds per setting; target rate × adaptation-rate grid; maximum bias fixed at 0.25 mV/ms.

## Screening results

Values are mean ± sample standard deviation across eight seeds.

| Target | Adaptation | Held-out | Retention | Reversal | Mean firing | Mean absolute bias |
|---:|---:|---:|---:|---:|---:|---:|
| 5 Hz | 0.001 | 0.988 ± 0.027 | 1.000 ± 0.000 | 0.891 ± 0.208 | 22.03 Hz | 0.232 |
| 5 Hz | 0.005 | **0.991 ± 0.027** | 0.991 ± 0.019 | **1.000 ± 0.000** | 21.83 Hz | 0.235 |
| 10 Hz | 0.001 | 0.872 ± 0.237 | 0.875 ± 0.231 | 1.000 ± 0.000 | 22.36 Hz | 0.243 |
| 10 Hz | 0.005 | 0.763 ± 0.239 | 0.759 ± 0.217 | 0.844 ± 0.297 | 22.18 Hz | 0.249 |
| 15 Hz | 0.001 | 0.938 ± 0.177 | 0.938 ± 0.177 | 0.853 ± 0.276 | 22.77 Hz | 0.212 |
| 15 Hz | 0.005 | 0.722 ± 0.304 | 0.719 ± 0.315 | 0.900 ± 0.171 | 22.50 Hz | 0.219 |
| 20 Hz | 0.001 | 0.903 ± 0.174 | 0.897 ± 0.192 | 0.969 ± 0.088 | 23.30 Hz | 0.240 |
| 20 Hz | 0.005 | 0.900 ± 0.193 | 0.903 ± 0.189 | 0.703 ± 0.271 | 23.12 Hz | 0.247 |

For comparison, reward-modulated STDP without homeostasis in the paired baseline had held-out accuracy 0.994 ± 0.018, retention 1.000 ± 0.000, and reversal 0.691 ± 0.237.

The 5 Hz / 0.005 setting preserved acquisition and retention almost exactly while reversal evaluation reached 1.0 on every seed. The paired mean differences versus reward-STDP alone were approximately −0.003 held-out accuracy, −0.009 retention, and +0.309 reversal accuracy.

## Caveats

The mean absolute bias for the best setting is 0.235 mV/ms against a 0.25 mV/ms cap, so many neurons may be close to saturation. The network-wide firing rate is still about 21.8 Hz despite a 5 Hz target. The controller therefore has not demonstrated convergence to its target; the observed benefit may be due to bounded activity suppression or altered excitability rather than successful rate homeostasis.

Eight seeds are still a screening sample. Do not promote these values to biological constants or a permanent default yet.

## Next: bias-cap sweep

The **Aliva homeostasis bias-cap sweep** workflow holds the promising target and adaptation rate fixed (5 Hz and 0.005) and tests maximum bias caps of 0.05, 0.10, 0.15, and 0.25 mV/ms over paired seeds. This should reveal whether the reversal benefit survives with less bias saturation.

The workflow uploads raw trial records and an aggregate CSV as `aliva-homeostasis-bias-cap-sweep`.


## Bias-cap follow-up

**Run:** [GitHub Actions bias-cap sweep #45937185855](https://github.com/ImTasell/Aliva/actions/runs/45937185855)  
**Artifact:** `aliva-homeostasis-bias-cap-sweep`  
**Fixed settings:** target 5 Hz, adaptation 0.005 mV/ms per Hz-second, rate-estimator tau 1000 ms; eight paired seeds.

| Maximum bias | Held-out | Retention | Reversal | Mean firing | Mean absolute bias |
|---:|---:|---:|---:|---:|---:|
| 0.05 mV/ms | 0.916 ± 0.179 | 0.909 ± 0.183 | 0.750 ± 0.220 | 24.88 Hz | 0.050 |
| 0.10 mV/ms | 0.938 ± 0.131 | 0.928 ± 0.152 | 0.813 ± 0.235 | 24.09 Hz | 0.100 |
| 0.15 mV/ms | 0.906 ± 0.178 | 0.909 ± 0.174 | 0.988 ± 0.035 | 23.26 Hz | 0.146 |
| 0.25 mV/ms | **0.991 ± 0.027** | **0.991 ± 0.019** | **1.000 ± 0.000** | 21.83 Hz | 0.235 |

The 0.25 mV/ms cap was best across the measured task metrics in this eight-seed screen. The 0.15 cap is a useful lower-activity-suppression alternative: it retained 0.988 reversal accuracy but had lower held-out accuracy (0.906). The smaller caps kept mean firing closer to baseline but gave up more reversal performance.

The 0.25 result still has a saturation warning: mean absolute bias (0.235) is 94% of the configured cap on average, while the network mean firing rate (21.83 Hz) remains far above the 5 Hz target. The controller is not demonstrating convergence to the requested target. Treat 0.25 as the best task-specific setting tested so far, not as a validated biological homeostasis setting or a new global default.

Next validation should use at least 32 paired seeds for the 0.15 and 0.25 caps against reward-STDP without homeostasis. Also record the fraction of neurons at the bias limit and firing rates separately for input, hidden, and output populations.
