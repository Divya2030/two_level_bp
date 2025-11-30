# Two-Level Adaptive Training Branch Prediction – Project Report

## 1. Introduction

- Motivation: branch prediction, deep pipelines, misprediction penalty.
- Goal: Reproduce and study Yeh and Patt's Two-Level Adaptive Training (AT) scheme.
- Outline of the rest of the report.

## 2. Paper Summary

### 2.1 Problem and Context

- Role of branches in pipelined and superscalar processors.
- Static vs dynamic branch prediction.
- Why higher accuracy is important (mis-speculation flush cost).

### 2.2 Two-Level Adaptive Training (AT)

- First level: per-address History Register Table (HRT) with k-bit shift registers.
- Second level: global Pattern Table (PT) with 2^k entries.
- Each PT entry stores a finite-state machine (Last-Time, A2, A3, A4).
- Prediction function A(S_c) and update function δ(S_c, R_i,c).

### 2.3 Variants and Alternatives

- IHRT (ideal, no interference) vs AHRT vs HHRT (Section 3.1).
- Different history lengths k.
- Comparison baselines:
  - Always-Taken
  - Bimodal 2-bit per-branch
  - Optionally Static Training, if implemented.

## 3. Simulator Design

### 3.1 Code Organization

- include/
  - types.hpp: Outcome enum.
  - stats.hpp: Stats struct (total, correct, accuracy).
  - automaton.hpp: Last-Time, A2, A3, A4 finite-state machines.
  - hrt.hpp: IHRT, AHRT, HHRT implementations.
  - pattern_table.hpp: Pattern table PT(2^k, Automaton).
  - at_config.hpp: Configuration struct for AT schemes.
  - two_level_at.hpp: TwoLevelATPredictor class.
  - predictors.hpp: AlwaysTaken and Bimodal2Bit baselines.
- src/
  - main.cpp: experiment driver with multiple AT configs and baselines.
  - hrt.cpp: implementation of IHRT/AHRT/HHRT.
  - pattern_table.cpp: implementation of PT.
  - two_level_at.cpp: implementation of Two-Level AT.
- analysis/
  - aggregate_results.py: collects CSV from logs into results.csv.
  - plot_results.py: generate plots from results.csv.
- report/
  - report_template.md: this template.

### 3.2 Mapping to the Paper

Explain how:

- HRT (IHRT, AHRT, HHRT) maps to Section 3.1.
- PT and state machines (Last-Time, A2, A3, A4) map to Section 2.1 and Fig. 2.
- The combined predictor matches Fig. 1 (Two-Level AT).
- main.cpp corresponds to Section 4's simulation methodology.

## 4. Methodology

### 4.1 Trace Format

- Each dynamic conditional branch is logged as:
  - "<pc_hex> <taken_bit>"
    - taken_bit = 1 for taken, 0 for not-taken.
- This approximates the dynamic conditional branch stream in the paper.

### 4.2 Evaluated Configurations

Describe the evaluated AT schemes:

- HRT variations:
  - AHRT(256, 4-way), AHRT(512, 4-way)
  - HHRT(256), HHRT(512)
  - IHRT
- History lengths:
  - k = 6, 8, 10, 12
- Pattern-table state machines:
  - Last-Time
  - A2
  - A3
  - A4

Baselines:

- AlwaysTaken
- Bimodal2Bit (2-bit per-branch saturating counters).

### 4.3 Data Collection and Plotting

Example workflow:

- Run all benchmarks:

  - ./bp_sim traces/eqntott.txt eqntott >> all_logs.txt
  - ./bp_sim traces/espresso.txt espresso >> all_logs.txt
  - ./bp_sim traces/gcc.txt gcc >> all_logs.txt
  - ./bp_sim traces/li.txt li >> all_logs.txt

- Aggregate:

  - cd analysis
  - python3 aggregate_results.py ../all_logs.txt

- Plot:

  - python3 plot_results.py

Explain that:

- accuracy_by_benchmark.png shows accuracy per scheme per benchmark.
- accuracy_by_scheme.png shows geometric mean accuracy for each scheme.

## 5. Results

### 5.1 Accuracy by Benchmark

- Insert accuracy_by_benchmark.png.
- Discuss trends:
  - AT vs. AlwaysTaken vs. Bimodal2Bit.
  - IHRT as upper bound vs AHRT vs HHRT.
  - Integer vs floating-point benchmarks (if applicable).

### 5.2 Geometric Mean Accuracy

- Insert accuracy_by_scheme.png.
- Compare overall rankings of schemes.
- Relate to the original paper qualitatively (e.g., AT ~97%, baselines below).

### 5.3 Hardware Cost vs Accuracy (Optional)

- Use hw_bits from the CSV.
- Make a table or plot (not provided in the scripts, but you can add one).
- Discuss trade-offs between hardware cost and prediction accuracy.

## 6. Conclusions and Future Work

- Summarize:
  - Effectiveness of Two-Level AT.
  - Impact of HRT design and k on accuracy.
- Future work ideas:
  - Implement full Static Training.
  - Implement slightly different A3/A4 automata.
  - Integrate into a cycle-level simulator (e.g., gem-style core) to evaluate IPC.

## 7. References

- Tse-Yu Yeh and Yale N. Patt, "Two-Level Adaptive Training Branch Prediction", MICRO-24, 1991.
