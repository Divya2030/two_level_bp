/*
 * Two-Level Adaptive Training Branch Predictor Simulator
 * ------------------------------------------------------
 * Based on:
 *   Tse-Yu Yeh and Yale N. Patt,
 *   "Two-Level Adaptive Training Branch Prediction",
 *   MICRO-24, 1991.
 *
 * This project is intentionally structured and commented so that each
 * component maps cleanly to sections and figures of the paper:
 *
 *   - Section 2 & Fig. 1:
 *       Two-Level AT structure: HRT(kSR) + PT(2^k, Automaton)
 *
 *   - Section 2.1:
 *       * History registers: shift registers holding last k outcomes.
 *       * Pattern history table: per-history FSM storing pattern behavior.
 *
 *   - Section 3.1:
 *       * IHRT: Ideal history register table (no interference).
 *       * AHRT: Set-associative history table (4-way).
 *       * HHRT: Hash-based history table.
 *
 *   - Section 4.2:
 *       * Initialization of histories and pattern-table entries.
 *
 *   - Section 5:
 *       * Experiment evaluation across multiple benchmarks and
 *         configurations (different HRT sizes, automata, and k).
 *
 * Trace format:
 *   Each line of the input trace is:
 *       <pc_hex> <taken_bit_0_or_1>
 *
 * Example:
 *       0x401000 1
 *       0x401004 0
 *
 * Command line:
 *   ./bp_sim trace.txt benchmark_name
 *
 * The benchmark_name is only used as a label in the CSV output so that
 * you can aggregate results across multiple traces.
 */

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "two_level_at.hpp"
#include "predictors.hpp"
#include "stats.hpp"

using namespace bp;

int main(int argc, char** argv) {
    // ------------------------------------------------------------
    //  Argument parsing & trace file opening (Section 4: Methodology)
    // ------------------------------------------------------------
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " trace.txt [benchmark_name]\n";
        std::cerr << "Each trace line: <pc_hex> <taken_bit_0_or_1>\n";
        return 1;
    }

    const std::string trace_file = argv[1];
    const std::string benchmark  = (argc >= 3) ? argv[2] : "unknown";

    std::ifstream in(trace_file);
    if (!in) {
        std::cerr << "Error: could not open trace file '" << trace_file << "'\n";
        return 1;
    }

    // ------------------------------------------------------------
    //  Define Two-Level AT configurations (like Table 2 and Figs. 5–7)
    // ------------------------------------------------------------
    std::vector<ATConfig> configs;

    // === HRT implementation exploration (similar to Fig. 6) ===
    //
    // Vary HRT type and size while keeping:
    //   - k   = 12 bits of history
    //   - PT  = 2^12 entries
    //   - FSM = A2
    configs.push_back({"AT_AHRT_256_12_A2", HRTKind::AHRT, 256, 4, 12, AutomatonType::A2});
    configs.push_back({"AT_AHRT_512_12_A2", HRTKind::AHRT, 512, 4, 12, AutomatonType::A2});
    configs.push_back({"AT_HHRT_256_12_A2", HRTKind::HHRT, 256, 1, 12, AutomatonType::A2});
    configs.push_back({"AT_HHRT_512_12_A2", HRTKind::HHRT, 512, 1, 12, AutomatonType::A2});
    configs.push_back({"AT_IHRT_12_A2",     HRTKind::IHRT,   0, 0, 12, AutomatonType::A2});

    // === Automaton exploration (similar to Fig. 5) ===
    //
    // Keep HRT = AHRT(512, 12SR) constant and vary automaton:
    configs.push_back({"AT_AHRT_512_12_LT", HRTKind::AHRT, 512, 4, 12, AutomatonType::LastTime});
    configs.push_back({"AT_AHRT_512_12_A3", HRTKind::AHRT, 512, 4, 12, AutomatonType::A3});
    configs.push_back({"AT_AHRT_512_12_A4", HRTKind::AHRT, 512, 4, 12, AutomatonType::A4});

    // === History length exploration (similar to Fig. 7) ===
    //
    // Fix HRT = AHRT(512) and automaton = A2, vary k:
    configs.push_back({"AT_AHRT_512_10_A2", HRTKind::AHRT, 512, 4, 10, AutomatonType::A2});
    configs.push_back({"AT_AHRT_512_8_A2",  HRTKind::AHRT, 512, 4,  8, AutomatonType::A2});
    configs.push_back({"AT_AHRT_512_6_A2",  HRTKind::AHRT, 512, 4,  6, AutomatonType::A2});

    // Wrap each config in an object that holds:
    //   - The config itself
    //   - A TwoLevelATPredictor
    //   - Stats for that predictor
    struct ATSim {
        ATConfig            cfg;
        TwoLevelATPredictor pred;
        Stats               stats;

        ATSim(const ATConfig& c) : cfg(c), pred(c), stats{} {}
    };

    std::vector<ATSim> at_sims;
    at_sims.reserve(configs.size());
    for (const auto& c : configs) {
        at_sims.emplace_back(c);
    }

    // ------------------------------------------------------------
    //  Baseline predictors: Always-Taken & Bimodal 2-bit
    // ------------------------------------------------------------
    AlwaysTakenPredictor always;
    Bimodal2BitPredictor bimodal;
    Stats stats_always;
    Stats stats_bimodal;

    // ------------------------------------------------------------
    //  Main trace-driven simulation loop (Section 4)
    // ------------------------------------------------------------
    //
    // For each dynamic branch:
    //   1. Parse PC and taken/not-taken from trace.
    //   2. For each AT scheme:
    //        - Predict
    //        - Compare to actual
    //        - Update PT and HRT
    //   3. For each baseline:
    //        - Same pattern (predict, compare, update)
    //
    std::uint64_t pc;
    int taken_int;

    while (true) {
        // Read PC in hexadecimal.
        if (!(in >> std::hex >> pc)) break;
        // Read outcome in decimal (0 or 1).
        if (!(in >> std::dec >> taken_int)) break;

        Outcome o = taken_int ? Outcome::Taken : Outcome::NotTaken;

        // --- Two-Level AT variants ---
        for (auto& sim : at_sims) {
            bool p = sim.pred.predict(pc);
            bool correct = (p && o == Outcome::Taken) ||
                           (!p && o == Outcome::NotTaken);
            if (correct) sim.stats.correct++;
            sim.stats.total++;
            sim.pred.update(pc, o);
        }

        // --- Always-Taken baseline ---
        {
            bool p = always.predict(pc);
            bool correct = (p && o == Outcome::Taken) ||
                           (!p && o == Outcome::NotTaken);
            if (correct) stats_always.correct++;
            stats_always.total++;
            always.update(pc, o);
        }

        // --- Bimodal 2-bit per-branch baseline ---
        {
            bool p = bimodal.predict(pc);
            bool correct = (p && o == Outcome::Taken) ||
                           (!p && o == Outcome::NotTaken);
            if (correct) stats_bimodal.correct++;
            stats_bimodal.total++;
            bimodal.update(pc, o);
        }
    }

    // ------------------------------------------------------------
    //  Human-readable summary (similar to paper's result sections)
    // ------------------------------------------------------------
    std::cout << "Trace file: " << trace_file << "\n";
    std::cout << "Benchmark:  " << benchmark  << "\n\n";

    std::cout << "=== Two-Level Adaptive Training (AT) Schemes ===\n\n";
    std::cout << std::fixed << std::setprecision(2);

    for (const auto& sim : at_sims) {
        double acc = sim.stats.accuracy() * 100.0;
        std::size_t hw = sim.pred.hardware_cost_bits();
        std::cout << sim.cfg.name << "\n";
        std::cout << "  Total branches:   " << sim.stats.total   << "\n";
        std::cout << "  Correct predicts: " << sim.stats.correct << "\n";
        std::cout << "  Accuracy:         " << acc << " %\n";
        std::cout << "  HW cost (approx): " << hw  << " bits\n\n";
    }

    std::cout << "=== Baseline Predictors ===\n\n";
    std::cout << "AlwaysTaken\n";
    std::cout << "  Total branches:   " << stats_always.total   << "\n";
    std::cout << "  Correct predicts: " << stats_always.correct << "\n";
    std::cout << "  Accuracy:         " << (stats_always.accuracy() * 100.0) << " %\n\n";

    std::cout << "Bimodal2Bit\n";
    std::cout << "  Total branches:   " << stats_bimodal.total   << "\n";
    std::cout << "  Correct predicts: " << stats_bimodal.correct << "\n";
    std::cout << "  Accuracy:         " << (stats_bimodal.accuracy() * 100.0) << " %\n\n";

    // ------------------------------------------------------------
    //  CSV output for analysis/aggregate_results.py & plot_results.py
    // ------------------------------------------------------------
    //
    // This block is easy to parse and matches the layout expected by
    // analysis/aggregate_results.py.
    //
    std::cout << "=== CSV (copy/paste into analysis/results.csv) ===\n";
    std::cout << "benchmark,scheme,total,correct,accuracy,hw_bits\n";

    for (const auto& sim : at_sims) {
        double acc = sim.stats.accuracy() * 100.0;
        std::size_t hw = sim.pred.hardware_cost_bits();
        std::cout << benchmark << ","
                  << sim.cfg.name << ","
                  << sim.stats.total << ","
                  << sim.stats.correct << ","
                  << acc << ","
                  << hw << "\n";
    }

    // Baseline rows: hw_bits = 0 (we treat them as "no AT hardware").
    std::cout << benchmark << ",AlwaysTaken,"
              << stats_always.total << ","
              << stats_always.correct << ","
              << (stats_always.accuracy() * 100.0) << ",0\n";

    std::cout << benchmark << ",Bimodal2Bit,"
              << stats_bimodal.total << ","
              << stats_bimodal.correct << ","
              << (stats_bimodal.accuracy() * 100.0) << ",0\n";

    return 0;
}
