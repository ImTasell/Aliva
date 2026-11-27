#include "aliva/association/config.hpp"
#include "aliva/association/export.hpp"
#include "aliva/association/experiment.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace aliva::assoc;

struct Options {
    Mode mode{Mode::RewardSTDP};
    std::uint64_t seed{1};
    int num_seeds{1};
    std::string out_prefix; // empty -> derived from mode/seed
    ExportFormat format{ExportFormat::Csv};
    bool help{false};

    // Optional overrides of the task schedule.
    int train_episodes{-1};
    int trials_per_episode{-1};
    int eval_trials{-1};

    // Optional overrides of plasticity / network parameters (for tuning).
    double learning_rate{-1.0};
    double potentiation{-1.0};
    double depression{-1.0};
    double eligibility_tau{-1.0};
    double out_weight{-1.0};
    double inhibition{-1.0};
    double hidden_weight{-1.0};
    double jitter{-1.0};
    int settle{-1};
    int rev_episodes{-1};
    int rev_trials{-1};
    int rev_eval{-1};
    int retention{-1};
    int idle{-1};
    double noise{-1.0};
    double maxw{-1.0};
};

void print_usage(const char* program) {
    std::cout <<
        "Aliva associative-learning benchmark\n\n"
        "Usage: " << program << " [options]\n\n"
        "Experiment:\n"
        "  --mode <fixed|reward|shuffled>  Learning condition (default: reward)\n"
        "  --seed <n>                      Base RNG seed (default: 1)\n"
        "  --seeds <n>                     Run n consecutive seeds (default: 1)\n"
        "  --format <csv|jsonl>            Per-trial output format (default: csv)\n"
        "  --out <prefix>                  Output path prefix\n"
        "  --episodes <n>                  Training episodes override\n"
        "  --trials <n>                    Trials per episode override\n"
        "  --eval <n>                      Held-out evaluation trials override\n"
        "  --rev-episodes <n>              Reversal training episodes override\n"
        "  --rev-trials <n>                Reversal trials per episode override\n"
        "  --rev-eval <n>                  Reversal evaluation trials override\n"
        "  --retention <n>                 Retention trials override\n"
        "  --idle <n>                      Retention idle steps override\n"
        "\nTuning (advanced):\n"
        "  --lr <f> --pot <f> --dep <f>    Plasticity learning rate / STDP amplitudes\n"
        "  --elig-tau <f>                  Eligibility time constant (ms)\n"
        "  --maxw <f>                      Maximum absolute weight (mV)\n"
        "  --out-weight <f>                Initial hidden->output weight (mV)\n"
        "  --hidden-weight <f>             Input->hidden weight (mV)\n"
        "  --inh <f>                       Output mutual-inhibition weight (mV)\n"
        "  --jitter <f>                    Initial weight jitter fraction [0,1)\n"
        "  --noise <f>                     Background exploration noise (mV/ms)\n"
        "  --settle <n>                    Read-out window steps\n"
        "  -h, --help                      Show this help\n\n"
        "Each run writes <prefix>_trials.<ext> and <prefix>_summary.json.\n";
}

Mode parse_mode(const std::string& text) {
    if (text == "fixed") return Mode::Fixed;
    if (text == "reward" || text == "reward_stdp") return Mode::RewardSTDP;
    if (text == "shuffled" || text == "shuffled_reward") return Mode::ShuffledReward;
    throw std::invalid_argument("Unknown mode: " + text);
}

ExportFormat parse_format(const std::string& text) {
    if (text == "csv") return ExportFormat::Csv;
    if (text == "jsonl") return ExportFormat::Jsonl;
    throw std::invalid_argument("Unknown format: " + text);
}

Options parse_args(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument(std::string("Missing value for ") + name);
            }
            return argv[++i];
        };
        if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "--mode") {
            options.mode = parse_mode(next("--mode"));
        } else if (arg == "--seed") {
            options.seed = static_cast<std::uint64_t>(std::stoull(next("--seed")));
        } else if (arg == "--seeds") {
            options.num_seeds = std::stoi(next("--seeds"));
            if (options.num_seeds < 1) {
                throw std::invalid_argument("--seeds must be at least 1.");
            }
        } else if (arg == "--format") {
            options.format = parse_format(next("--format"));
        } else if (arg == "--out") {
            options.out_prefix = next("--out");
        } else if (arg == "--episodes") {
            options.train_episodes = std::stoi(next("--episodes"));
        } else if (arg == "--trials") {
            options.trials_per_episode = std::stoi(next("--trials"));
        } else if (arg == "--eval") {
            options.eval_trials = std::stoi(next("--eval"));
        } else if (arg == "--lr") {
            options.learning_rate = std::stod(next("--lr"));
        } else if (arg == "--pot") {
            options.potentiation = std::stod(next("--pot"));
        } else if (arg == "--dep") {
            options.depression = std::stod(next("--dep"));
        } else if (arg == "--elig-tau") {
            options.eligibility_tau = std::stod(next("--elig-tau"));
        } else if (arg == "--out-weight") {
            options.out_weight = std::stod(next("--out-weight"));
        } else if (arg == "--inh") {
            options.inhibition = std::stod(next("--inh"));
        } else if (arg == "--hidden-weight") {
            options.hidden_weight = std::stod(next("--hidden-weight"));
        } else if (arg == "--jitter") {
            options.jitter = std::stod(next("--jitter"));
        } else if (arg == "--settle") {
            options.settle = std::stoi(next("--settle"));
        } else if (arg == "--rev-episodes") {
            options.rev_episodes = std::stoi(next("--rev-episodes"));
        } else if (arg == "--rev-trials") {
            options.rev_trials = std::stoi(next("--rev-trials"));
        } else if (arg == "--rev-eval") {
            options.rev_eval = std::stoi(next("--rev-eval"));
        } else if (arg == "--retention") {
            options.retention = std::stoi(next("--retention"));
        } else if (arg == "--idle") {
            options.idle = std::stoi(next("--idle"));
        } else if (arg == "--noise") {
            options.noise = std::stod(next("--noise"));
        } else if (arg == "--maxw") {
            options.maxw = std::stod(next("--maxw"));
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }
    return options;
}

Config make_config(const Options& options) {
    Config config;
    config.mode = options.mode;
    config.seed = options.seed;
    if (options.train_episodes > 0) config.task.train_episodes = options.train_episodes;
    if (options.trials_per_episode > 0) config.task.trials_per_episode = options.trials_per_episode;
    if (options.eval_trials > 0) config.task.eval_trials = options.eval_trials;

    if (options.learning_rate >= 0.0) config.plasticity.learning_rate_mV = options.learning_rate;
    if (options.potentiation >= 0.0) config.plasticity.potentiation = options.potentiation;
    if (options.depression >= 0.0) config.plasticity.depression = options.depression;
    if (options.eligibility_tau > 0.0) config.plasticity.eligibility_tau_ms = options.eligibility_tau;
    if (options.out_weight > 0.0) config.network.hidden_to_output_weight_mV = options.out_weight;
    if (options.inhibition >= 0.0) config.network.output_inhibition_weight_mV = options.inhibition;
    if (options.hidden_weight > 0.0) config.network.input_to_hidden_weight_mV = options.hidden_weight;
    if (options.jitter >= 0.0) config.network.hidden_to_output_weight_jitter = options.jitter;
    if (options.settle > 0) config.timing.settle_steps = options.settle;
    if (options.rev_episodes > 0) config.task.reversal_episodes = options.rev_episodes;
    if (options.rev_trials > 0) config.task.reversal_trials_per_episode = options.rev_trials;
    if (options.rev_eval > 0) config.task.reversal_eval_trials = options.rev_eval;
    if (options.retention > 0) config.task.retention_trials = options.retention;
    if (options.idle >= 0) config.task.retention_idle_steps = options.idle;
    if (options.noise >= 0.0) config.timing.background_noise_mV_per_ms = options.noise;
    if (options.maxw > 0.0) config.plasticity.max_abs_weight_mV = options.maxw;
    return config;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_args(argc, argv);
        if (options.help) {
            print_usage(argv[0]);
            return 0;
        }

        double eval_sum = 0.0;
        int eval_count = 0;

        for (int s = 0; s < options.num_seeds; ++s) {
            const std::uint64_t seed = options.seed + static_cast<std::uint64_t>(s);
            Config config = make_config(options);
            config.seed = seed;

            Experiment experiment(config, seed);
            experiment.run();
            const Summary& summary = experiment.summary();

            std::string prefix = options.out_prefix;
            if (prefix.empty()) {
                prefix = "association_" + to_string(config.mode) + "_seed" +
                         std::to_string(seed);
            } else if (options.num_seeds > 1) {
                prefix += "_seed" + std::to_string(seed);
            }

            export_results(prefix, options.format, config, experiment.records(), summary);

            eval_sum += summary.eval_accuracy;
            ++eval_count;

            std::cout << "mode=" << to_string(config.mode)
                      << " seed=" << seed
                      << " train=" << summary.train_accuracy
                      << " eval=" << summary.eval_accuracy
                      << " reversal_eval=" << summary.reversal_eval_accuracy
                      << " retention=" << summary.retention_accuracy
                      << " trials_to_threshold=" << summary.trials_to_threshold
                      << " neurons=" << summary.neuron_count
                      << " synapses=" << summary.synapse_count
                      << " -> " << prefix << "_trials\n";
        }

        if (options.num_seeds > 1) {
            std::cout << "mean held-out accuracy over " << eval_count
                      << " seeds: " << (eval_sum / static_cast<double>(eval_count)) << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}