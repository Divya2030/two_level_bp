"""
plot_results.py

Generate plots from results.csv produced by aggregate_results.py.

Input:
    results.csv with columns:
        benchmark,scheme,total,correct,accuracy,hw_bits

Output:
    accuracy_by_benchmark.png  - per-benchmark grouped bar chart
    accuracy_by_scheme.png     - per-scheme geometric mean accuracy
"""

import math
from collections import defaultdict, OrderedDict

import matplotlib.pyplot as plt

CSV_FILE = "results.csv"

# ---------- Load data from CSV (robustly) ----------

data = defaultdict(dict)      # benchmark -> scheme -> accuracy
scheme_to_accs = defaultdict(list)
benchmarks_order = []
schemes_order = []

with open(CSV_FILE, "r") as f:
    first = True
    for raw in f:
        line = raw.strip()
        if not line:
            continue
        # Skip header
        if first:
            first = False
            if line.startswith("benchmark,scheme"):
                continue

        # Skip obviously non-CSV or malformed lines
        if line.startswith("Trace file:") or line.startswith("Benchmark:") or line.startswith("==="):
            continue

        parts = line.split(",")
        if len(parts) != 6:
            # Not a valid row
            continue

        bench, scheme, total, correct, acc_str, hw_bits = [p.strip() for p in parts]
        if not bench or not scheme or not acc_str:
            continue

        try:
            acc = float(acc_str)
        except ValueError:
            continue

        data[bench][scheme] = acc
        scheme_to_accs[scheme].append(acc)

        if bench not in benchmarks_order:
            benchmarks_order.append(bench)
        if scheme not in schemes_order:
            schemes_order.append(scheme)


# ---------- Plot 1: accuracy by benchmark (grouped bars) ----------

if not benchmarks_order or not schemes_order:
    print("No valid data found in results.csv to plot.")
    raise SystemExit(1)

x = range(len(benchmarks_order))
width = 0.8 / max(1, len(schemes_order))  # bar width

plt.figure(figsize=(10, 5))

for idx, scheme in enumerate(schemes_order):
    offsets = [i + (idx - len(schemes_order) / 2) * width for i in x]
    y = [data[bench].get(scheme, 0.0) for bench in benchmarks_order]
    plt.bar(offsets, y, width, label=scheme)

plt.xticks(list(x), benchmarks_order, rotation=45, ha="right")
plt.ylabel("Prediction accuracy (%)")
plt.title("Branch prediction accuracy by benchmark and scheme")
plt.ylim(0, 100)
plt.legend(fontsize="small", ncol=2)
plt.tight_layout()
plt.savefig("accuracy_by_benchmark.png", dpi=300)

# ---------- Plot 2: geometric mean accuracy per scheme ----------

def geometric_mean(values):
    """
    Compute geometric mean of percentages, where 'values' is a list of
    values in [0, 100]. Convert to [0, 1], average logs, exponentiate,
    then convert back to percentage.
    """
    vals = [v / 100.0 for v in values if v > 0.0]
    if not vals:
        return 0.0
    logs = [math.log(v) for v in vals]
    return math.exp(sum(logs) / len(logs)) * 100.0


schemes_gmean = OrderedDict()
for scheme in schemes_order:
    g = geometric_mean(scheme_to_accs[scheme])
    schemes_gmean[scheme] = g

plt.figure(figsize=(8, 4))
x2 = range(len(schemes_gmean))
y2 = list(schemes_gmean.values())
plt.bar(x2, y2)
plt.xticks(list(x2), list(schemes_gmean.keys()), rotation=45, ha="right")
plt.ylabel("Geometric mean accuracy (%)")
plt.title("Geometric mean accuracy per scheme")
plt.ylim(0, 100)
plt.tight_layout()
plt.savefig("accuracy_by_scheme.png", dpi=300)

