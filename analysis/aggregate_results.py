"""
aggregate_results.py

Collect CSV blocks emitted by multiple runs of bp_sim and merge them
into a single results.csv file.

Usage:

    python3 aggregate_results.py ../all_logs.txt

It looks for blocks that start with:

    === CSV (copy/paste into analysis/results.csv) ===

and then reads only *valid* CSV data lines with 6 comma-separated fields:
    benchmark,scheme,total,correct,accuracy,hw_bits
"""

import sys
import os

HEADER = "benchmark,scheme,total,correct,accuracy,hw_bits"


def extract_from_file(path):
    """
    Scan a bp_sim log file and pull out all CSV data lines from blocks
    that start with the CSV marker. We *only* keep lines that look like
    proper CSV rows (6 comma-separated fields).
    """
    rows = []
    in_csv = False

    with open(path, "r") as f:
        for raw in f:
            line = raw.strip()
            if not line:
                # blank line terminates a CSV block
                in_csv = False
                continue

            if line.startswith("=== CSV"):
                in_csv = True
                continue

            if not in_csv:
                continue

            # Skip header line from bp_sim
            if line.startswith("benchmark,scheme"):
                continue

            # Ignore lines that are clearly not CSV rows
            if line.startswith("Trace file:") or line.startswith("Benchmark:"):
                in_csv = False
                continue

            # Check if it's a 6-field CSV row
            parts = line.split(",")
            if len(parts) != 6:
                # This probably means we've left the CSV area
                in_csv = False
                continue

            rows.append(line)

    return rows


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 aggregate_results.py <log1> [<log2> ...]")
        sys.exit(1)

    all_rows = []
    for path in sys.argv[1:]:
        if not os.path.exists(path):
            print(f"Warning: file '{path}' not found, skipping.")
            continue
        all_rows.extend(extract_from_file(path))

    if not all_rows:
        print("No CSV rows found in provided logs.")
        sys.exit(1)

    out_path = "results.csv"
    with open(out_path, "w") as out:
        out.write(HEADER + "\n")
        for r in all_rows:
            out.write(r + "\n")

    print(f"Wrote {len(all_rows)} rows to {out_path}")


if __name__ == "__main__":
    main()

