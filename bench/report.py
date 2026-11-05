"""Aggregate downloaded summary*.json artifacts into a markdown report."""
import glob
import json


def main():
    files = sorted(glob.glob("all/**/summary*.json", recursive=True) + glob.glob("all/summary*.json"))
    print("# TgCrypto: original (1.2.5) vs this (v1.3.1)\n")
    if not files:
        print("_No benchmark artifacts found._")
        return
    for f in files:
        s = json.load(open(f))
        o, n = s["orig"], s["new"]
        print(f"## {s['os']}\n")
        print(f"original {s['orig_ver']}  ->  this {s['new_ver']}\n")
        print("| metric | original MB/s | this MB/s | speedup |")
        print("|---|---:|---:|---:|")
        for sec in ("bulk", "small_64B"):
            for k in o[sec]:
                ov, nv = o[sec][k], n[sec][k]
                print(f"| {sec}/{k} | {ov:.1f} | {nv:.1f} | {nv / ov:.2f}x |")
        print()


if __name__ == "__main__":
    main()
