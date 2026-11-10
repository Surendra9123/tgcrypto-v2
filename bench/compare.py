#!/usr/bin/env python3
"""Build ``summary.json`` from three benchmark JSON files.

Inputs (all optional, produced by bench.py):
    orig.json  - original PyPI tgcrypto 1.2.5       (BENCH_JSON)
    new.json   - this fork / accelerated build      (BENCH_JSON)
    kuri.json  - kuricrypt                           (KURI_JSON)

Output:
    summary.json with per-operation throughput for each implementation and
    speedup ratios (this/orig, this/kuri).
"""

import json
import os

OPS = [
    "ige_encrypt", "ige_decrypt",
    "ctr",
    "cbc_encrypt", "cbc_decrypt",
    "small_ctr", "small_ige", "small_cbc",
]


def _load(path):
    if not os.path.exists(path):
        return None
    with open(path) as f:
        data = json.load(f)
    # tolerate both raw dict and nested {"results": {...}}
    if isinstance(data, dict) and "results" in data and isinstance(data["results"], dict):
        return data["results"]
    return data


def main():
    orig = _load("orig.json")
    new = _load("new.json")
    kuri = _load("kuri.json")

    rows = []
    for op in OPS:
        row = {"operation": op}
        o = (orig or {}).get(op)
        n = (new or {}).get(op)
        k = (kuri or {}).get(op)
        row["original"] = o
        row["this"] = n
        row["kuricrypt"] = k
        row["speedup_vs_original"] = round(n / o, 2) if (n and o) else None
        row["speedup_vs_kuricrypt"] = round(n / k, 2) if (n and k) else None
        rows.append(row)

    summary = {
        "columns": ["original", "this", "kuricrypt"],
        "rows": rows,
    }
    with open("summary.json", "w") as f:
        json.dump(summary, f, indent=2)
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
