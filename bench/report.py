#!/usr/bin/env python3
"""Aggregate every ``summary.json`` found under ``all/`` and print a table.

Each summary.json (one per OS in the CI matrix) already contains the
original / this / kuricrypt throughput columns; this script just groups them
by platform label and prints a readable report.
"""

import json
import os
import sys


def _find_summaries(root="all"):
    found = []
    for dirpath, _dirnames, filenames in os.walk(root):
        if "summary.json" in filenames:
            found.append(os.path.join(dirpath, "summary.json"))
    return sorted(found)


def _fmt(v):
    if v is None:
        return "-"
    return "%.1f" % v


def _fmt_x(v):
    if v is None:
        return "-"
    return "%.2fx" % v


def main():
    summaries = _find_summaries()
    if not summaries:
        print("No summary.json files found under ./all", file=sys.stderr)
        return 1

    for path in summaries:
        with open(path) as f:
            data = json.load(f)
        label = os.path.basename(os.path.dirname(path))
        print("\n=== %s ===" % label)
        header = "%-12s %10s %10s %10s %12s %14s" % (
            "operation", "original", "this", "kuricrypt",
            "this/orig", "this/kuri")
        print(header)
        print("-" * len(header))
        for row in data.get("rows", []):
            print("%-12s %10s %10s %10s %12s %14s" % (
                row["operation"],
                _fmt(row.get("original")),
                _fmt(row.get("this")),
                _fmt(row.get("kuricrypt")),
                _fmt_x(row.get("speedup_vs_original")),
                _fmt_x(row.get("speedup_vs_kuricrypt")),
            ))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
