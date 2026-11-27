# Homeostasis parameter sweep

The initial M2 comparison used the core defaults (5 Hz target, 1000 ms rate-estimator time constant, 0.05 adaptation rate, and 1.0 mV/ms maximum bias). That setting reduced network firing substantially and degraded task accuracy, so it should be treated as an overly aggressive first configuration—not as evidence against homeostasis generally.

The **Aliva homeostasis parameter sweep** GitHub Actions workflow tests reward-modulated STDP with eight combinations:

- Target firing rates: 5, 10, 15, and 20 Hz.
- Adaptation rates: 0.001 and 0.005 mV/ms per Hz-second.
- Maximum absolute bias: fixed at 0.25 mV/ms.
- Rate-estimator time constant: fixed at 1000 ms.
- Eight paired seeds by default (configurable when dispatching).

Each run uses the same task seeds. The generated summary JSON records all homeostasis parameters, and `comparison.csv` groups results by the complete parameter tuple. The workflow uploads raw trial records and aggregate statistics as `aliva-homeostasis-sweep`.

This is a small screening sweep, not a full parameter optimization. Follow promising settings with more seeds and inspect per-population firing rates before choosing a default.
