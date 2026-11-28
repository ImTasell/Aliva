#include "aliva/association/export.hpp"

#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <utility>

namespace aliva::assoc {
namespace {

std::ofstream open_or_throw(const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Could not open output file: " + path);
    }
    return out;
}

} // namespace

std::string to_string(ExportFormat format) {
    switch (format) {
        case ExportFormat::Csv: return "csv";
        case ExportFormat::Jsonl: return "jsonl";
    }
    return "unknown";
}

void write_csv(const std::string& path, const Config& config, const std::vector<TrialRecord>& records) {
    std::ofstream out = open_or_throw(path);
    out << "mode,seed,trial,episode,phase,cue,correct_response,selected_response,"
           "reward,correct,output0_spikes,output1_spikes,total_spikes,sim_time_ms\n";
    out << std::setprecision(6);
    for (const TrialRecord& rec : records) {
        out << to_string(config.mode) << ','
            << config.seed << ','
            << rec.trial << ','
            << rec.episode << ','
            << to_string(rec.phase) << ','
            << (rec.cue == 0 ? "A" : "B") << ','
            << rec.correct_response << ','
            << rec.selected_response << ','
            << rec.reward << ','
            << (rec.correct ? 1 : 0) << ','
            << rec.output0_spikes << ','
            << rec.output1_spikes << ','
            << rec.total_spikes << ','
            << rec.sim_time_ms << '\n';
    }
}

void write_jsonl(const std::string& path, const Config& config, const std::vector<TrialRecord>& records) {
    std::ofstream out = open_or_throw(path);
    out << std::setprecision(6);
    for (const TrialRecord& rec : records) {
        out << "{\"mode\":\"" << to_string(config.mode) << "\","
            << "\"seed\":" << config.seed << ','
            << "\"trial\":" << rec.trial << ','
            << "\"episode\":" << rec.episode << ','
            << "\"phase\":\"" << to_string(rec.phase) << "\","
            << "\"cue\":\"" << (rec.cue == 0 ? "A" : "B") << "\","
            << "\"correct_response\":" << rec.correct_response << ','
            << "\"selected_response\":" << rec.selected_response << ','
            << "\"reward\":" << rec.reward << ','
            << "\"correct\":" << (rec.correct ? "true" : "false") << ','
            << "\"output0_spikes\":" << rec.output0_spikes << ','
            << "\"output1_spikes\":" << rec.output1_spikes << ','
            << "\"total_spikes\":" << rec.total_spikes << ','
            << "\"sim_time_ms\":" << rec.sim_time_ms << '}' << '\n';
    }
}

void write_summary_json(const std::string& path, const Summary& summary) {
    std::ofstream out = open_or_throw(path);
    out << std::setprecision(6);
    out << "{\n";
    out << "  \"mode\": \"" << to_string(summary.mode) << "\",\n";
    out << "  \"seed\": " << summary.seed << ",\n";
    out << "  \"train_accuracy\": " << summary.train_accuracy << ",\n";
    out << "  \"eval_accuracy\": " << summary.eval_accuracy << ",\n";
    out << "  \"trials_to_threshold\": " << summary.trials_to_threshold << ",\n";
    out << "  \"reversal_eval_accuracy\": " << summary.reversal_eval_accuracy << ",\n";
    out << "  \"retention_accuracy\": " << summary.retention_accuracy << ",\n";
    out << "  \"total_trials\": " << summary.total_trials << ",\n";
    out << "  \"total_spikes\": " << summary.total_spikes << ",\n";
    out << "  \"mean_firing_rate_hz\": " << summary.mean_firing_rate_hz << ",\n";
    out << "  \"mean_abs_homeostatic_bias_mV_per_ms\": " << summary.mean_abs_homeostatic_bias_mV_per_ms << ",\n";
    out << "  \"homeostatic_bias_saturation_fraction\": " << summary.homeostatic_bias_saturation_fraction << ",\n";
    auto write_population = [&](const char* name, const PopulationStats& population) {
        out << "  \"" << name << "\": {\n";
        out << "    \"neuron_count\": " << population.neuron_count << ",\n";
        out << "    \"mean_firing_rate_hz\": " << population.mean_firing_rate_hz << ",\n";
        out << "    \"firing_rate_stddev_hz\": " << population.firing_rate_stddev_hz << ",\n";
        out << "    \"mean_abs_homeostatic_bias_mV_per_ms\": "
            << population.mean_abs_homeostatic_bias_mV_per_ms << ",\n";
        out << "    \"bias_saturation_fraction\": "
            << population.bias_saturation_fraction << "\n";
        out << "  },\n";
    };
    write_population("input_population", summary.input_population);
    write_population("hidden_population", summary.hidden_population);
    write_population("output_population", summary.output_population);
    out << "  \"homeostasis_target_firing_rate_hz\": " << summary.homeostasis_target_firing_rate_hz << ",\n";
    out << "  \"homeostasis_rate_time_constant_ms\": " << summary.homeostasis_rate_time_constant_ms << ",\n";
    out << "  \"homeostasis_adaptation_rate\": " << summary.homeostasis_adaptation_rate << ",\n";
    out << "  \"homeostasis_max_abs_bias_mV_per_ms\": " << summary.homeostasis_max_abs_bias_mV_per_ms << ",\n";
    out << "  \"neuron_count\": " << summary.neuron_count << ",\n";
    out << "  \"synapse_count\": " << summary.synapse_count << ",\n";
    out << "  \"episode_accuracy\": [";
    for (std::size_t i = 0; i < summary.episode_accuracy.size(); ++i) {
        out << (i == 0 ? "" : ", ") << summary.episode_accuracy[i];
    }
    out << "],\n";
    out << "  \"reversal_episode_accuracy\": [";
    for (std::size_t i = 0; i < summary.reversal_episode_accuracy.size(); ++i) {
        out << (i == 0 ? "" : ", ") << summary.reversal_episode_accuracy[i];
    }
    out << "],\n";
    out << "  \"output_weights\": {\n";
    out << "    \"count\": " << summary.output_weights.count << ",\n";
    out << "    \"min\": " << summary.output_weights.min << ",\n";
    out << "    \"max\": " << summary.output_weights.max << ",\n";
    out << "    \"mean\": " << summary.output_weights.mean << ",\n";
    out << "    \"mean_abs\": " << summary.output_weights.mean_abs << "\n";
    out << "  }\n";
    out << "}\n";
}

void export_results(
    const std::string& prefix,
    ExportFormat format,
    const Config& config,
    const std::vector<TrialRecord>& records,
    const Summary& summary)
{
    const std::string ext = (format == ExportFormat::Csv) ? ".csv" : ".jsonl";
    const std::string trials_path = prefix + "_trials" + ext;
    if (format == ExportFormat::Csv) {
        write_csv(trials_path, config, records);
    } else {
        write_jsonl(trials_path, config, records);
    }
    write_summary_json(prefix + "_summary.json", summary);
}

} // namespace aliva::assoc