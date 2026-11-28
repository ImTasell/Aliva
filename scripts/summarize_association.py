#!/usr/bin/env python3
"""Summarize paired-seed Aliva associative-learning benchmark outputs."""

from __future__ import annotations

import csv
import json
import math
import pathlib
import statistics
import sys
from collections import defaultdict


def nested(data: dict, path: str):
    value = data
    for part in path.split("."):
        value = value[part]
    return float(value)


def mean_std(values: list[float]) -> tuple[float, float]:
    if not values:
        return math.nan, math.nan
    return statistics.mean(values), statistics.stdev(values) if len(values) > 1 else 0.0


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: summarize_association.py RESULTS_DIR OUTPUT.csv", file=sys.stderr)
        return 2

    results_dir = pathlib.Path(sys.argv[1])
    output_path = pathlib.Path(sys.argv[2])
    summaries = sorted(results_dir.glob("*_summary.json"))
    if not summaries:
        raise SystemExit(f"No *_summary.json files found in {results_dir}")

    grouped: dict[tuple, list[dict]] = defaultdict(list)
    for path in summaries:
        with path.open(encoding="utf-8") as stream:
            summary = json.load(stream)
        key = (summary["mode"], summary["homeostasis_target_firing_rate_hz"], summary["homeostasis_rate_time_constant_ms"], summary["homeostasis_adaptation_rate"], summary["homeostasis_max_abs_bias_mV_per_ms"])
        grouped[key].append(summary)

    fields = [
        "mode",
        "homeostasis_target_hz",
        "homeostasis_rate_tau_ms",
        "homeostasis_adaptation_rate",
        "homeostasis_max_abs_bias_mV_per_ms",
        "seed_count",
        "eval_accuracy_mean",
        "eval_accuracy_std",
        "retention_accuracy_mean",
        "retention_accuracy_std",
        "reversal_eval_accuracy_mean",
        "reversal_eval_accuracy_std",
        "trials_to_threshold_mean",
        "threshold_reached_fraction",
        "mean_firing_rate_hz_mean",
        "mean_firing_rate_hz_std",
        "mean_abs_homeostatic_bias_mean",
        "homeostatic_bias_saturation_fraction_mean",
        "homeostatic_bias_saturation_fraction_std",
        "input_mean_firing_rate_hz_mean",
        "input_mean_firing_rate_hz_std",
        "input_firing_rate_stddev_hz_mean",
        "input_mean_abs_homeostatic_bias_mV_per_ms_mean",
        "input_bias_saturation_fraction_mean",
        "hidden_mean_firing_rate_hz_mean",
        "hidden_mean_firing_rate_hz_std",
        "hidden_firing_rate_stddev_hz_mean",
        "hidden_mean_abs_homeostatic_bias_mV_per_ms_mean",
        "hidden_bias_saturation_fraction_mean",
        "output_mean_firing_rate_hz_mean",
        "output_mean_firing_rate_hz_std",
        "output_firing_rate_stddev_hz_mean",
        "output_mean_abs_homeostatic_bias_mV_per_ms_mean",
        "output_bias_saturation_fraction_mean",
        "output_weight_mean_abs_mean",
    ]

    rows = []
    for (mode, target_hz, rate_tau_ms, adaptation_rate, max_bias), runs in sorted(grouped.items()):
        eval_mean, eval_std = mean_std([nested(run, "eval_accuracy") for run in runs])
        retention_mean, retention_std = mean_std([nested(run, "retention_accuracy") for run in runs])
        reversal_mean, reversal_std = mean_std([nested(run, "reversal_eval_accuracy") for run in runs])
        firing_mean, firing_std = mean_std([nested(run, "mean_firing_rate_hz") for run in runs])
        reached = [nested(run, "trials_to_threshold") for run in runs if nested(run, "trials_to_threshold") >= 0]
        bias_mean, _ = mean_std([nested(run, "mean_abs_homeostatic_bias_mV_per_ms") for run in runs])
        saturation_mean, saturation_std = mean_std([
            nested(run, "homeostatic_bias_saturation_fraction") for run in runs
        ])
        population_metrics = {}
        for population in ("input", "hidden", "output"):
            prefix = f"{population}_population"
            rate_mean, rate_std = mean_std([
                nested(run, f"{prefix}.mean_firing_rate_hz") for run in runs
            ])
            within_run_rate_std, _ = mean_std([
                nested(run, f"{prefix}.firing_rate_stddev_hz") for run in runs
            ])
            population_bias_mean, _ = mean_std([
                nested(run, f"{prefix}.mean_abs_homeostatic_bias_mV_per_ms") for run in runs
            ])
            population_saturation_mean, _ = mean_std([
                nested(run, f"{prefix}.bias_saturation_fraction") for run in runs
            ])
            population_metrics.update({
                f"{population}_mean_firing_rate_hz_mean": rate_mean,
                f"{population}_mean_firing_rate_hz_std": rate_std,
                f"{population}_firing_rate_stddev_hz_mean": within_run_rate_std,
                f"{population}_mean_abs_homeostatic_bias_mV_per_ms_mean": population_bias_mean,
                f"{population}_bias_saturation_fraction_mean": population_saturation_mean,
            })
        weight_mean, _ = mean_std([nested(run, "output_weights.mean_abs") for run in runs])
        rows.append({
            "mode": mode,
            "homeostasis_target_hz": target_hz,
            "homeostasis_rate_tau_ms": rate_tau_ms,
            "homeostasis_adaptation_rate": adaptation_rate,
            "homeostasis_max_abs_bias_mV_per_ms": max_bias,
            "seed_count": len(runs),
            "eval_accuracy_mean": eval_mean,
            "eval_accuracy_std": eval_std,
            "retention_accuracy_mean": retention_mean,
            "retention_accuracy_std": retention_std,
            "reversal_eval_accuracy_mean": reversal_mean,
            "reversal_eval_accuracy_std": reversal_std,
            "trials_to_threshold_mean": statistics.mean(reached) if reached else math.nan,
            "threshold_reached_fraction": len(reached) / len(runs),
            "mean_firing_rate_hz_mean": firing_mean,
            "mean_firing_rate_hz_std": firing_std,
            "mean_abs_homeostatic_bias_mean": bias_mean,
            "homeostatic_bias_saturation_fraction_mean": saturation_mean,
            "homeostatic_bias_saturation_fraction_std": saturation_std,
            **population_metrics,
            "output_weight_mean_abs_mean": weight_mean,
        })

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote {output_path}")
    for row in rows:
        print(
            f"{row['mode']}: eval={row['eval_accuracy_mean']:.3f}"
            f" ± {row['eval_accuracy_std']:.3f} (n={row['seed_count']})"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
