# Two-Level Adaptive Training Branch Predictor (Yeh & Patt, MICRO-24)

This repository is a implementation of:

Tse-Yu Yeh and Yale N. Patt, "Two-Level Adaptive Training Branch Prediction", MICRO-24, 1991.

It implements:

- The Two-Level Adaptive Training (AT) branch predictor from the paper
- Multiple HRT implementations (IHRT, AHRT, HHRT) – Section 3.1
- Pattern-table finite state machines (Last-Time, A2, A3, A4) – Fig. 2
- Simple baselines: Always Taken, Bimodal 2-bit
- Scripts to generate evaluation graphs similar to the paper

## 1. Build

### With g++ (simplest)

From repo root:

    g++ -std=c++17 -O2 \
        src/main.cpp src/hrt.cpp src/pattern_table.cpp src/two_level_at.cpp \
        -Iinclude -o bp_sim

This produces `./bp_sim`.

### With CMake (optional)

If you want to use CMake, first install it (Ubuntu):

    sudo apt update
    sudo apt install cmake

Then:

    mkdir -p build
    cd build
    cmake ..
    make

Executable: `build/bp_sim`.

## 2. Trace Format

Each line = one dynamic conditional branch:

    <pc_hex> <taken_bit_0_or_1>

Example:

    0x401000 1
    0x401004 0
    0x401000 1

- `pc_hex` = static branch PC
- `taken_bit`:
  - `1` = branch taken
  - `0` = branch not taken

## 3. Running the Simulator

From repo root (or from `build` if you used CMake):

    ./bp_sim path/to/trace.txt benchmark_name

Example:

    ./bp_sim trace.txt toybench

The program prints:
- Detailed stats for multiple AT configurations (different HRTs, k, automata)
- Baseline stats (AlwaysTaken, Bimodal2Bit)
- A CSV block at the end for later plotting.

## 4. Analysis Scripts

After running several benchmarks and saving logs (e.g. to `all_logs.txt`):

    cd analysis
    python3 aggregate_results.py ../all_logs.txt
    python3 plot_results.py

This will create:

- `analysis/results.csv`
- `analysis/accuracy_by_benchmark.png`
- `analysis/accuracy_by_scheme.png`

## 5. Mapping Code to Paper

- `include/hrt.hpp`, `src/hrt.cpp`:
  IHRT / AHRT / HHRT (Section 3.1)

- `include/pattern_table.hpp`, `src/pattern_table.cpp`:
  PT(2^k, Automaton) (Fig. 1 & Section 2.1)

- `include/automaton.hpp`:
  Last-Time, A2, A3, A4 (Fig. 2)

- `include/two_level_at.hpp`, `src/two_level_at.cpp`:
  Two-Level AT structure (Section 2 & 2.1)

- `src/main.cpp`:
  Experiment driver (Section 4 & 5 style evaluation)

