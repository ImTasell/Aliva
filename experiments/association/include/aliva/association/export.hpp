#pragma once

#include "aliva/association/config.hpp"
#include "aliva/association/experiment.hpp"

#include <string>
#include <vector>

namespace aliva::assoc {

enum class ExportFormat { Csv, Jsonl };

[[nodiscard]] std::string to_string(ExportFormat format);

// Write one row per trial. `path` is the destination file (created/truncated).
void write_csv(const std::string& path, const Config& config, const std::vector<TrialRecord>& records);

// Write one JSON object per trial (newline-delimited JSON).
void write_jsonl(const std::string& path, const Config& config, const std::vector<TrialRecord>& records);

// Write the aggregate summary as a single JSON object.
void write_summary_json(const std::string& path, const Summary& summary);

// Convenience wrapper that writes both the per-trial file and the summary file
// next to it. `prefix` is a path prefix; the extension is added automatically.
void export_results(
    const std::string& prefix,
    ExportFormat format,
    const Config& config,
    const std::vector<TrialRecord>& records,
    const Summary& summary);

} // namespace aliva::assoc