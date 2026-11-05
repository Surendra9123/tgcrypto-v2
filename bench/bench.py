"""Throughput benchmark for tgcrypto. Prints JSON to stdout.

Works with both the original (software-only) tgcrypto 1.2.5 and the
hardware-accelerated build in this repo, since the public API is identical.
"""
import json
import os
import platform
import sys
import time

import tgcrypto


def _bench(fn, nbytes, iters):
    data = bytes(os.urandom(nbytes))
    fn(data)  # warmup
    best = None
    for _ in range(iters):
        t0 = time.perf_counter()
        fn(data)
        dt = time.perf_counter() - t0
        if best is None or dt < best:
            best = dt
    return nbytes / best / 1e6  # MB/s


def main():
    key = bytes(os.urandom(32))
    iv16 = bytes(os.urandom(16))
    iv32 = bytes(os.urandom(32))
    state = b"\x00"

    BIG = 8 * 1024 * 1024
    BIG_ITERS = 3
    SMALL = 64
    SMALL_CALLS = 1024 * 1024 // SMALL  # 1 MiB total

    res = {
        "python": platform.python_version(),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "tgcrypto_version": getattr(tgcrypto, "__version__", "unknown"),
    }

    # Bulk (MB/s)
    res["bulk"] = {
        "cbc_encrypt": _bench(lambda d: tgcrypto.cbc256_encrypt(d, key, iv16), BIG, BIG_ITERS),
        "cbc_decrypt": _bench(lambda d: tgcrypto.cbc256_decrypt(d, key, iv16), BIG, BIG_ITERS),
        "ctr_encrypt": _bench(lambda d: tgcrypto.ctr256_encrypt(d, key, iv16, state), BIG, BIG_ITERS),
        "ige_encrypt": _bench(lambda d: tgcrypto.ige256_encrypt(d, key, iv32), BIG, BIG_ITERS),
        "ige_decrypt": _bench(lambda d: tgcrypto.ige256_decrypt(d, key, iv32), BIG, BIG_ITERS),
    }

    # Small-payload: measure per-call throughput over many calls (MB/s)
    small = bytes(os.urandom(SMALL))

    def small_bench(fn):
        for _ in range(SMALL_CALLS):
            fn(small)  # warmup
        best = None
        for _ in range(3):
            t0 = time.perf_counter()
            for _ in range(SMALL_CALLS):
                fn(small)
            dt = time.perf_counter() - t0
            if best is None or dt < best:
                best = dt
        return (SMALL * SMALL_CALLS) / best / 1e6

    res["small_64B"] = {
        "cbc_encrypt": small_bench(lambda d: tgcrypto.cbc256_encrypt(d, key, iv16)),
        "ctr_encrypt": small_bench(lambda d: tgcrypto.ctr256_encrypt(d, key, iv16, state)),
        "ige_encrypt": small_bench(lambda d: tgcrypto.ige256_encrypt(d, key, iv32)),
    }

    print("BENCH_JSON:" + json.dumps(res))


if __name__ == "__main__":
    main()
