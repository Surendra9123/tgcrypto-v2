"""Compare orig.json vs new.json benchmark outputs, print a table, write summary.json."""
import json


def main():
    o = json.load(open("orig.json"))
    n = json.load(open("new.json"))
    print("OS:", n["platform"], "| py", n["python"])
    print(f"{'metric':20s}{'original':>12s}{'this':>12s}{'speedup':>10s}")

    def row(sec, k):
        ov = o[sec][k]
        nv = n[sec][k]
        print(f"{sec + '/' + k:20s}{ov:12.1f}{nv:12.1f}{nv / ov:9.2f}x")

    for k in o["bulk"]:
        row("bulk", k)
    for k in o["small_64B"]:
        row("small_64B", k)

    summary = {
        "os": n["platform"],
        "orig_ver": o["tgcrypto_version"],
        "new_ver": n["tgcrypto_version"],
        "orig": o,
        "new": n,
    }
    open("summary.json", "w").write(json.dumps(summary))


if __name__ == "__main__":
    main()
