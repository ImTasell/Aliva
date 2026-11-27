// Tests for the associative-learning benchmark harness
// (experiments/association). These exercise the experiment layer built on the
// public Brain API; they do not modify or depend on the neural core's internals.

#include "aliva/association/config.hpp"
#include "aliva/association/decoding.hpp"
#include "aliva/association/export.hpp"
#include "aliva/association/experiment.hpp"
#include "aliva/association/network.hpp"
#include "aliva/association/patterns.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace aliva::assoc;

struct TestCase {
    const char* name;
    std::function<void()> run;
};

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Exception, typename Callable>
void require_throws(Callable&& callable, const std::string& message) {
    try {
        callable();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error(message);
}

std::filesystem::path temp_path(const std::string& name) {
    return std::filesystem::temp_directory_path() / ("aliva_assoc_test_" + name);
}

// --- Seeded input patterns -------------------------------------------------

void cue_patterns_are_seeded_and_distinct() {
    std::mt19937_64 rng_a{123};
    const CuePatterns first = make_cue_patterns(24, 6, 3.0, rng_a);

    std::mt19937_64 rng_b{123};
    const CuePatterns second = make_cue_patterns(24, 6, 3.0, rng_b);

    require(first.inputs(Cue::A) == second.inputs(Cue::A),
            "Same seed must reproduce cue A pattern.");
    require(first.inputs(Cue::B) == second.inputs(Cue::B),
            "Same seed must reproduce cue B pattern.");

    // The two cues must be disjoint so the network can separate them.
    for (const std::size_t a : first.inputs(Cue::A)) {
        for (const std::size_t b : first.inputs(Cue::B)) {
            require(a != b, "Cue patterns must be disjoint.");
        }
    }

    std::mt19937_64 rng_c{999};
    const CuePatterns other = make_cue_patterns(24, 6, 3.0, rng_c);
    require(other.inputs(Cue::A) != first.inputs(Cue::A) ||
                other.inputs(Cue::B) != first.inputs(Cue::B),
            "A different seed should generally give a different pattern.");
}

void cue_patterns_reject_invalid_sizes() {
    std::mt19937_64 rng{1};
    require_throws<std::invalid_argument>([&] { (void)make_cue_patterns(2, 3, 3.0, rng); },
        "active_per_cue larger than num_input/2 must be rejected.");
}

// --- Output decoding -------------------------------------------------------

void decoding_is_deterministic() {
    // With equal first-spike latency, more spikes wins.
    require(decode_response(5, 2, 2, 2, -60.0, -60.0).response == 0,
            "More output-0 spikes should select response 0.");
    require(decode_response(1, 4, 2, 2, -60.0, -60.0).response == 1,
            "More output-1 spikes should select response 1.");

    // First-spike latency takes priority over the spike count.
    require(decode_response(5, 2, 3, 1, -60.0, -60.0).response == 1,
            "Earlier first spike should win even with fewer spikes.");
    require(decode_response(3, 3, 2, 5, -60.0, -60.0).response == 0,
            "Earlier first spike should win a count tie.");

    // A silent neuron loses to a spiking one.
    require(decode_response(0, 3, -1, 4, -65.0, -65.0).response == 1,
            "A spiking output should beat a silent one.");

    // Neither spiked: the more depolarised neuron wins; exact tie -> 0.
    require(decode_response(0, 0, -1, -1, -55.0, -58.0).response == 0,
            "Higher membrane potential should win when neither spiked.");
    require(decode_response(0, 0, -1, -1, -60.0, -60.0).response == 0,
            "An exact tie must resolve deterministically to response 0.");
}

// --- Network construction --------------------------------------------------

void network_size_matches_configuration() {
    Config config;
    config.network.num_input = 12;
    config.network.num_hidden = 40;
    std::mt19937_64 rng{7};
    aliva::Brain brain;
    const NetworkHandles handles =
        build_network(brain, config.network, make_cue_patterns(12, 3, 3.0, rng), Mode::RewardSTDP, rng);

    require(brain.neuron_count() == 12 + 40 + 2,
            "Neuron count must equal input + hidden + output.");
    require(handles.input.size() == 12 && handles.hidden.size() == 40,
            "Handles must cover every population.");

    // Divergent read-out: each hidden neuron projects to exactly one output.
    for (const aliva::NeuronId hidden : handles.hidden) {
        require(brain.synapses_from(hidden).size() == 1,
                "Each hidden neuron must project to a single output neuron.");
    }
    // Total synapses = (input->hidden) + 40 (hidden->output) + 2 (inhibitory).
    require(brain.synapse_count() > 40 + 2,
            "Sparse input->hidden connectivity must add synapses on top of the read-out.");
    require(brain.synapse_count() >= 40 + 2,
            "Read-out and inhibitory synapses must exist.");
}

// --- Weight behaviour by mode ---------------------------------------------

void fixed_mode_never_changes_weights() {
    Config config;
    config.mode = Mode::Fixed;
    Experiment experiment(config, 1);
    const std::vector<double> before = experiment.hidden_to_output_weights();
    experiment.run();
    const std::vector<double> after = experiment.hidden_to_output_weights();
    require(before == after, "Fixed baseline weights must never change.");
}

void reward_mode_changes_weights() {
    Config config;
    config.mode = Mode::RewardSTDP;
    Experiment experiment(config, 1);
    const std::vector<double> before = experiment.hidden_to_output_weights();
    experiment.run();
    const std::vector<double> after = experiment.hidden_to_output_weights();
    require(before != after, "Reward-modulated STDP must change weights during training.");
}

void ordinary_stdp_mode_changes_weights_without_reward() {
    Config config;
    config.mode = Mode::OrdinarySTDP;
    Experiment experiment(config, 1);
    const auto before = experiment.hidden_to_output_weights();
    experiment.run();
    const auto after = experiment.hidden_to_output_weights();
    require(before != after,
            "Ordinary STDP must modify weights from spike timing without task reward.");
    for (const TrialRecord& record : experiment.records()) {
        require(record.reward == 0.0,
                "Ordinary STDP must not receive task reward or a modulatory signal.");
    }
}

void reward_homeostasis_mode_is_explicit() {
    Config config;
    config.mode = Mode::RewardSTDPHomeostasis;
    Experiment experiment(config, 1);
    require(experiment.config().mode == Mode::RewardSTDPHomeostasis,
            "Homeostatic reward mode must remain explicit in experiment metadata.");
    experiment.run();
    require(experiment.summary().mean_firing_rate_hz >= 0.0,
            "Homeostatic experiment must report a finite non-negative firing rate.");
    require(experiment.summary().mean_abs_homeostatic_bias_mV_per_ms > 0.0,
            "Homeostasis mode must actually adapt intrinsic bias during training.");
}

void evaluation_phases_supply_zero_reward() {
    Config config;
    config.mode = Mode::RewardSTDP;
    Experiment experiment(config, 1);
    experiment.run();
    for (const TrialRecord& rec : experiment.records()) {
        if (rec.phase == Phase::Eval || rec.phase == Phase::Retention ||
            rec.phase == Phase::ReversalEval) {
            require(rec.reward == 0.0,
                    "Evaluation phases must apply exactly zero reward (no weight change).");
        }
    }
}

// --- Determinism / reproducibility ----------------------------------------

void same_seed_reproduces_the_run() {
    Config config;
    config.mode = Mode::RewardSTDP;
    Experiment first(config, 42);
    Experiment second(config, 42);
    first.run();
    second.run();

    const auto& a = first.records();
    const auto& b = second.records();
    require(a.size() == b.size(), "Same seed must produce the same number of trials.");
    for (std::size_t i = 0; i < a.size(); ++i) {
        require(a[i].trial == b[i].trial && a[i].cue == b[i].cue &&
                    a[i].selected_response == b[i].selected_response &&
                    a[i].reward == b[i].reward && a[i].correct == b[i].correct,
                "Same seed must reproduce every trial outcome.");
    }
    require(first.hidden_to_output_weights() == second.hidden_to_output_weights(),
            "Same seed must reproduce the final weights.");
}

void paired_seed_preserves_cue_schedule_across_modes() {
    Config reward_config;
    reward_config.mode = Mode::RewardSTDP;
    Config shuffled_config = reward_config;
    shuffled_config.mode = Mode::ShuffledReward;
    Config ordinary_config = reward_config;
    ordinary_config.mode = Mode::OrdinarySTDP;

    Experiment reward(reward_config, 1234);
    Experiment shuffled(shuffled_config, 1234);
    Experiment ordinary(ordinary_config, 1234);
    reward.run();
    shuffled.run();
    ordinary.run();

    const auto& expected = reward.records();
    const auto& shuffled_records = shuffled.records();
    const auto& ordinary_records = ordinary.records();
    require(expected.size() == shuffled_records.size() &&
                expected.size() == ordinary_records.size(),
            "Paired modes must have identical trial counts.");
    for (std::size_t i = 0; i < expected.size(); ++i) {
        require(expected[i].cue == shuffled_records[i].cue &&
                    expected[i].cue == ordinary_records[i].cue,
                "The same seed must preserve the cue schedule across learning modes.");
    }
}

void different_seed_changes_the_run() {
    Config config;
    config.mode = Mode::RewardSTDP;
    Experiment first(config, 1);
    Experiment second(config, 2);
    first.run();
    second.run();
    require(first.records().size() == second.records().size(),
            "Trial count is fixed by the schedule, not the seed.");
    bool any_difference = false;
    for (std::size_t i = 0; i < first.records().size(); ++i) {
        if (first.records()[i].selected_response != second.records()[i].selected_response) {
            any_difference = true;
            break;
        }
    }
    require(any_difference, "Different seeds should generally differ in their responses.");
}

// --- Task-level learning claims -------------------------------------------

void reward_mode_learns_the_association() {
    Config config; // tuned defaults
    config.mode = Mode::RewardSTDP;
    Experiment experiment(config, 1);
    experiment.run();
    require(experiment.summary().eval_accuracy >= 0.90,
            "Reward-modulated STDP should exceed chance on held-out trials.");
    require(experiment.summary().retention_accuracy >= 0.90,
            "Learned association should be retained without training.");
}

void fixed_and_shuffled_controls_do_not_learn() {
    Config reward_config;
    reward_config.mode = Mode::RewardSTDP;
    Experiment reward(reward_config, 1);
    reward.run();

    Config fixed_config;
    fixed_config.mode = Mode::Fixed;
    Experiment fixed(fixed_config, 1);
    fixed.run();

    Config shuffled_config;
    shuffled_config.mode = Mode::ShuffledReward;
    Experiment shuffled(shuffled_config, 1);
    shuffled.run();

    const double reward_eval = reward.summary().eval_accuracy;
    require(reward_eval >= 0.90, "Reward condition should learn.");
    require(fixed.summary().eval_accuracy < reward_eval - 0.2,
            "Fixed baseline must not match the learning condition.");
    require(shuffled.summary().eval_accuracy < reward_eval - 0.2,
            "Shuffled-reward control must not match the learning condition.");
}

void reversal_learning_adapts() {
    Config config;
    config.mode = Mode::RewardSTDP;
    Experiment experiment(config, 1);
    experiment.run();

    const std::vector<double>& curve = experiment.summary().reversal_episode_accuracy;
    require(!curve.empty(), "Reversal curve must be recorded.");
    // The network must unlearn the old mapping (early reversal is poor) and
    // then adapt to the new mapping (some later episode is clearly above the
    // old-mapping baseline).
    double best = 0.0;
    for (const double value : curve) {
        best = std::max(best, value);
    }
    require(best >= 0.60,
            "Reversal should adapt to the new mapping (best episode >= 0.60).");
    require(curve.front() < best,
            "Reversal should start below its adapted level (old mapping interferes).");
}

// --- Summary consistency ---------------------------------------------------

void summary_is_consistent_with_records() {
    Config config;
    config.mode = Mode::RewardSTDP;
    Experiment experiment(config, 3);
    experiment.run();
    const Summary& s = experiment.summary();
    require(s.total_trials == experiment.records().size(),
            "Summary trial count must match the records.");
    require(s.neuron_count == experiment.neuron_count(),
            "Summary neuron count must match the network.");
    require(s.synapse_count == experiment.synapse_count(),
            "Summary synapse count must match the network.");
    require(s.output_weights.count == experiment.hidden_to_output_weights().size(),
            "Weight-stat count must match the number of read-out synapses.");
    require(s.mean_firing_rate_hz >= 0.0 && std::isfinite(s.mean_firing_rate_hz),
            "Firing rate must be finite and non-negative.");
}

// --- Export ----------------------------------------------------------------

void csv_export_writes_header_and_rows() {
    Config config;
    config.mode = Mode::RewardSTDP;
    Experiment experiment(config, 1);
    experiment.run();

    const std::filesystem::path path = temp_path("export.csv");
    write_csv(path.string(), config, experiment.records());

    std::ifstream in(path);
    require(static_cast<bool>(in), "CSV file should be created.");
    std::string line;
    std::getline(in, line);
    require(line.rfind("mode,seed,trial", 0) == 0, "CSV must start with the header.");
    std::size_t rows = 0;
    while (std::getline(in, line)) {
        if (!line.empty()) ++rows;
    }
    require(rows == experiment.records().size(),
            "CSV must contain exactly one row per trial.");
    std::filesystem::remove(path);
}

void jsonl_export_writes_one_object_per_trial() {
    Config config;
    config.mode = Mode::Fixed;
    Experiment experiment(config, 1);
    experiment.run();

    const std::filesystem::path path = temp_path("export.jsonl");
    write_jsonl(path.string(), config, experiment.records());

    std::ifstream in(path);
    require(static_cast<bool>(in), "JSONL file should be created.");
    std::string line;
    std::size_t rows = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        ++rows;
        require(line.front() == '{' && line.back() == '}',
                "Each JSONL line must be a JSON object.");
        require(line.find("\"trial\":") != std::string::npos,
                "Each JSONL object must contain a trial field.");
    }
    require(rows == experiment.records().size(),
            "JSONL must contain exactly one object per trial.");
    std::filesystem::remove(path);
}

// --- Configuration validation ---------------------------------------------

void invalid_configuration_is_rejected() {
    Config config;
    config.timing.settle_steps = 0;
    require_throws<std::invalid_argument>([&] { config.validate(); },
        "A non-positive settle window must be rejected.");

    Config overlapping;
    overlapping.network.num_input = 8;
    overlapping.network.active_inputs_per_cue = 6; // 2*6 > 8
    require_throws<std::invalid_argument>([&] { overlapping.validate(); },
        "Overlapping cue patterns must be rejected.");

    Config bad_mode = config;
    bad_mode.timing.settle_steps = 50;
    bad_mode.network.output_mutual_inhibition = true;
    bad_mode.network.output_inhibition_weight_mV = -1.0;
    require_throws<std::invalid_argument>([&] { bad_mode.validate(); },
        "Negative inhibition weight must be rejected.");
}

} // namespace

int main() {
    const std::vector<TestCase> tests{
        {"cue patterns are seeded and distinct", cue_patterns_are_seeded_and_distinct},
        {"cue patterns reject invalid sizes", cue_patterns_reject_invalid_sizes},
        {"decoding is deterministic", decoding_is_deterministic},
        {"network size matches configuration", network_size_matches_configuration},
        {"fixed mode never changes weights", fixed_mode_never_changes_weights},
        {"reward mode changes weights", reward_mode_changes_weights},
        {"ordinary STDP learns without reward", ordinary_stdp_mode_changes_weights_without_reward},
        {"reward homeostasis mode is explicit", reward_homeostasis_mode_is_explicit},
        {"evaluation phases supply zero reward", evaluation_phases_supply_zero_reward},
        {"same seed reproduces the run", same_seed_reproduces_the_run},
        {"paired seeds preserve cue schedule across modes", paired_seed_preserves_cue_schedule_across_modes},
        {"different seed changes the run", different_seed_changes_the_run},
        {"reward mode learns the association", reward_mode_learns_the_association},
        {"fixed and shuffled controls do not learn", fixed_and_shuffled_controls_do_not_learn},
        {"reversal learning adapts", reversal_learning_adapts},
        {"summary is consistent with records", summary_is_consistent_with_records},
        {"csv export writes header and rows", csv_export_writes_header_and_rows},
        {"jsonl export writes one object per trial", jsonl_export_writes_one_object_per_trial},
        {"invalid configuration is rejected", invalid_configuration_is_rejected},
    };

    std::size_t failures = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
        }
    }

    std::cout << tests.size() - failures << '/' << tests.size() << " tests passed.\n";
    return failures == 0 ? 0 : 1;
}