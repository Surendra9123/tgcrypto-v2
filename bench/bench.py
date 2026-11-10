#!/usr/bin/env python3
"""Benchmark TgCrypto (this build), the original PyPI tgcrypto, and kuricrypt.

Emits machine-readable lines so compare.py can build a summary:

    BENCH_JSON:{...}   benchmark of the *imported* ``tgcrypto`` module
    KURI_JSON:{...}    benchmark of ``kuricrypt`` (if it is importable)

The same benchmark routine (``benchmark_module``) is used for every
implementation so the numbers are apples-to-apples.  kuricrypt exposes the
same six-call API (ige/ctr/cbc, 256-bit) as tgcrypto, so it can be dropped in
transparently.
"""

import json
import os
import sys
import time

# ---- benchmark parameters -------------------------------------------------

BULK_SIZE = 8 * 1024 * 1024      # 8 MiB bulk buffers (as in the README tables)
SMALL_SIZE = 64                  # 64-byte payloads
REPEATS = 7                      # timed repetitions per operation
KEY_SIZE = 32                    # AES-256


def _throughput(nbytes, seconds):
    if seconds <= 0:
        return 0.0
    return (nbytes / seconds) / 1e6  # MB/s (decimal)


def _timeit(fn, *args):
    # warm-up
    fn(*args)
    best = None
    for _ in range(REPEATS):
        t0 = time.perf_counter()
        fn(*args)
        dt = time.perf_counter() - t0
        if best is None or dt < best:
            best = dt
    return best


def benchmark_module(mod):
    """Run the full benchmark suite against ``mod`` and return a dict.

    ``mod`` must expose the tgcrypto/kuricrypt 256-bit API:
    ige256_{en,de}crypt, ctr256_{en,de}crypt, cbc256_{en,de}crypt.
    """
    bulk = os.urandom(BULK_SIZE)
    small = os.urandom(SMALL_SIZE)

    key = os.urandom(KEY_SIZE)
    iv16 = os.urandom(16)
    iv32 = os.urandom(32)

    results = {}

    # ---- bulk (8 MiB) ----
    ops = {
        # ige uses a 32-byte IV
        "ige_encrypt": lambda: mod.ige256_encrypt(bulk, key, iv32),
        "ige_decrypt": lambda: mod.ige256_decrypt(_IGE_CT, key, iv32),
        # ctr uses a 16-byte IV + 1-byte state
        "ctr": lambda: mod.ctr256_encrypt(bulk, key, iv16, bytes(1)),
        # cbc uses a 16-byte IV
        "cbc_encrypt": lambda: mod.cbc256_encrypt(bulk, key, iv16),
        "cbc_decrypt": lambda: mod.cbc256_decrypt(_CBC_CT, key, iv16),
    }

    # Pre-compute ciphertexts for the decrypt benchmarks with this module
    global _IGE_CT, _CBC_CT
    _IGE_CT = mod.ige256_encrypt(bulk, key, iv32)
    _CBC_CT = mod.cbc256_encrypt(bulk, key, iv16)

    for name, fn in ops.items():
        dt = _timeit(fn)
        results[name] = round(_throughput(BULK_SIZE, dt), 1)

    # ---- small (64 B) ----
    small_ops = {
        "small_ctr": lambda: mod.ctr256_encrypt(small, key, iv16, bytes(1)),
        "small_ige": lambda: mod.ige256_encrypt(small, key, iv32),
        "small_cbc": lambda: mod.cbc256_encrypt(small, key, iv16),
    }
    for name, fn in small_ops.items():
        dt = _timeit(fn)
        results[name] = round(_throughput(SMALL_SIZE, dt), 1)

    return results


# Module-level placeholders populated inside benchmark_module
_IGE_CT = b""
_CBC_CT = b""


def main():
    emitted = False

    # Benchmark the *imported* tgcrypto (original 1.2.5 or this build).
    try:
        import tgcrypto  # type: ignore
    except Exception as exc:
        print("tgcrypto not available: %s" % exc, file=sys.stderr)
    else:
        tg = benchmark_module(tgcrypto)
        print("BENCH_JSON:" + json.dumps(tg, sort_keys=True))
        sys.stdout.flush()
        emitted = True

    # kuricrypt is optional; benchmark it when installed.
    try:
        import kuricrypt  # type: ignore
    except Exception as exc:
        print("kuricrypt not available: %s" % exc, file=sys.stderr)
    else:
        kuri = benchmark_module(kuricrypt)
        print("KURI_JSON:" + json.dumps(kuri, sort_keys=True))
        emitted = True

    if not emitted:
        print("no crypto backend importable", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
