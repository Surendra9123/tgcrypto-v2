# TgCrypto

> [!NOTE]
> The project is no longer maintained or supported. Thanks for appreciating it.

> [!NOTE]
> The implementations of the algorithms presented in this repository are to be considered for educational purposes only.

> Fast and Portable Cryptography Extension Library for Pyrogram

**TgCrypto** is a Cryptography Library written in C as a Python extension. It is designed to be portable, fast,
easy to install and use. TgCrypto is intended for [Pyrogram](https://github.com/pyrogram/pyrogram) and implements the
cryptographic algorithms Telegram requires, namely:

- **`AES-256-IGE`** - used in [MTProto v2.0](https://core.telegram.org/mtproto).
- **`AES-256-CTR`** - used for [CDN encrypted files](https://core.telegram.org/cdn).
- **`AES-256-CBC`** - used for [encrypted passport credentials](https://core.telegram.org/passport).

## Performance

This fork adds a **hardware-accelerated AES-256 backend** (ARMv8 Crypto Extensions on
Apple Silicon/ARM, AES-NI on x86 via GCC/Clang) with an automatic portable-software
fallback, and rewrites the mode loops (notably CTR, which now XORs full 16-byte blocks
instead of processing byte-by-byte).

All numbers below are produced by the **Benchmark Compare** GitHub Actions workflow
(`.github/workflows/benchmark-compare.yml`) on the actual CI runners — not local
measurements — and are regenerated on every push. Three implementations of the same
six-call API are compared with one harness (`bench/bench.py`):

- **original** — the original PyPI **tgcrypto 1.2.5** (pure-software AES)
- **this** — this accelerated build (**v1.3.1**)
- **kuricrypt** — [`kurigram-org/kuricrypt`](https://github.com/kurigram-org/kuricrypt), an independent **Rust** implementation

Bulk = 8 MiB buffers; small = 64-byte payloads. Throughput in MB/s (decimal).
`this/orig` and `this/kuri` are the speedup ratios of this build.

### macOS (Apple Silicon, arm64) — hardware AES

| Operation | original | this | kuricrypt | this/orig | this/kuri |
|---|---:|---:|---:|---:|---:|
| CBC encrypt | 171.1 | 595.5 | 601.7 | **3.48x** | **0.99x** |
| CBC decrypt | 146.6 | 1931.4 | 2679.0 | **13.17x** | **0.72x** |
| CTR | 125.0 | 2392.3 | 2180.1 | **19.14x** | **1.10x** |
| IGE encrypt | 139.8 | 537.7 | 681.1 | **3.85x** | **0.79x** |
| IGE decrypt | 130.2 | 388.1 | 621.6 | **2.98x** | **0.62x** |
| small 64B (CTR) | 69.8 | 153.5 | 118.3 | **2.20x** | **1.30x** |
| small 64B (IGE) | 90.4 | 153.8 | 128.0 | **1.70x** | **1.20x** |
| small 64B (CBC) | 90.4 | 153.5 | 139.7 | **1.70x** | **1.10x** |

### Linux (x86_64, glibc) — hardware AES-NI

| Operation | original | this | kuricrypt | this/orig | this/kuri |
|---|---:|---:|---:|---:|---:|
| CBC encrypt | 214.1 | 770.7 | 731.7 | **3.60x** | **1.05x** |
| CBC decrypt | 250.3 | 3194.3 | 6248.3 | **12.76x** | **0.51x** |
| CTR | 167.3 | 747.3 | 2013.1 | **4.47x** | **0.37x** |
| IGE encrypt | 211.3 | 760.0 | 835.1 | **3.60x** | **0.91x** |
| IGE decrypt | 212.5 | 760.3 | 837.2 | **3.58x** | **0.91x** |
| small 64B (CTR) | 72.6 | 86.4 | 122.8 | **1.19x** | **0.70x** |
| small 64B (IGE) | 94.0 | 99.8 | 212.6 | **1.06x** | **0.47x** |
| small 64B (CBC) | 95.4 | 103.1 | 187.7 | **1.08x** | **0.55x** |

### Windows (x64) — software fallback (MSVC)

MSVC does not support the function `target("aes")` attribute used to enable AES-NI
inlining, so the x86 hardware path is compiled only for GCC/Clang. Windows therefore
uses the portable software AES path; gains come from the faster mode loops and cheaper
key schedule only.

| Operation | original | this | kuricrypt | this/orig | this/kuri |
|---|---:|---:|---:|---:|---:|
| CBC encrypt | 154.9 | 171.3 | 639.4 | **1.11x** | **0.27x** |
| CBC decrypt | 180.7 | 204.7 | 2284.3 | **1.13x** | **0.09x** |
| CTR | 127.8 | 176.1 | 1780.1 | **1.38x** | **0.10x** |
| IGE encrypt | 135.2 | 165.6 | 649.3 | **1.22x** | **0.26x** |
| IGE decrypt | 136.8 | 168.9 | 643.4 | **1.23x** | **0.26x** |
| small 64B (CTR) | 49.2 | 58.2 | 80.0 | **1.18x** | **0.73x** |
| small 64B (IGE) | 64.0 | 71.1 | 128.0 | **1.11x** | **0.56x** |
| small 64B (CBC) | 71.1 | 71.1 | 106.7 | **1.00x** | **0.67x** |

> **TL;DR** — Against the original software tgcrypto 1.2.5 this build is a large win
> everywhere on hardware-AES platforms (bulk CTR/CBC-decrypt especially). Against
> **kuricrypt** (Rust, vectorised AES) the outcome is mixed: kuricrypt is faster on
> large-buffer bulk throughput, while this build tends to win on small 64-byte messages
> (the common MTProto framing case). See the per-OS tables above for exact figures.

> Reproduce with the **Benchmark Compare** GitHub Actions workflow
> (`.github/workflows/benchmark-compare.yml`); it benchmarks the original, this build,
> and kuricrypt with the same harness on ubuntu/macos/windows and emits a combined
> `summary.json`.

## Requirements

- Python 3.7 or higher.

## Installation

This fork (v1.3.1) is distributed via GitHub Releases and is **not** on PyPI under the
`tgcrypto` name (that name belongs to the original 1.2.5). Do not run `pip install tgcrypto`
if you want this accelerated build.

Install a prebuilt wheel (recommended) from the
[v1.3.1 release](https://github.com/Surendra9123/tgcrypto-v2/releases/tag/v1.3.1) — pick the
file matching your OS and Python, e.g.:

``` bash
$ pip3 install "tgcrypto @ https://github.com/Surendra9123/tgcrypto-v2/releases/download/v1.3.1/tgcrypto-1.3.1-cp313-cp313-win_amd64.whl"
```

Or build from source directly from the repo (requires a C compiler):

``` bash
$ pip3 install "git+https://github.com/Surendra9123/tgcrypto-v2.git@v1.3.1"
```

## API

TgCrypto API consists of these six methods:

```python
def ige256_encrypt(data: bytes, key: bytes, iv: bytes) -> bytes: ...
def ige256_decrypt(data: bytes, key: bytes, iv: bytes) -> bytes: ...

def ctr256_encrypt(data: bytes, key: bytes, iv: bytes, state: bytes) -> bytes: ...
def ctr256_decrypt(data: bytes, key: bytes, iv: bytes, state: bytes) -> bytes: ...

def cbc256_encrypt(data: bytes, key: bytes, iv: bytes) -> bytes: ...
def cbc256_decrypt(data: bytes, key: bytes, iv: bytes) -> bytes: ...
```

## Usage

### IGE Mode

**Note**: Data must be padded to match a multiple of the block size (16 bytes).

``` python
import os

import tgcrypto

data = os.urandom(10 * 1024 * 1024 + 7)  # 10 MB of random data + 7 bytes to show padding
key = os.urandom(32)  # Random Key
iv = os.urandom(32)  # Random IV

# Pad with zeroes: -7 % 16 = 9
data += bytes(-len(data) % 16)

ige_encrypted = tgcrypto.ige256_encrypt(data, key, iv)
ige_decrypted = tgcrypto.ige256_decrypt(ige_encrypted, key, iv)

print(data == ige_decrypted)  # True
```
    
### CTR Mode (single chunk)

``` python
import os

import tgcrypto

data = os.urandom(10 * 1024 * 1024)  # 10 MB of random data

key = os.urandom(32)  # Random Key

enc_iv = bytearray(os.urandom(16))  # Random IV
dec_iv = enc_iv.copy()  # Keep a copy for decryption

ctr_encrypted = tgcrypto.ctr256_encrypt(data, key, enc_iv, bytes(1))
ctr_decrypted = tgcrypto.ctr256_decrypt(ctr_encrypted, key, dec_iv, bytes(1))

print(data == ctr_decrypted)  # True
```

### CTR Mode (stream)

``` python
import os
from io import BytesIO

import tgcrypto

data = BytesIO(os.urandom(10 * 1024 * 1024))  # 10 MB of random data

key = os.urandom(32)  # Random Key

enc_iv = bytearray(os.urandom(16))  # Random IV
dec_iv = enc_iv.copy()  # Keep a copy for decryption

enc_state = bytes(1)  # Encryption state, starts from 0
dec_state = bytes(1)  # Decryption state, starts from 0

encrypted_data = BytesIO()  # Encrypted data buffer
decrypted_data = BytesIO()  # Decrypted data buffer

while True:
    chunk = data.read(1024)

    if not chunk:
        break

    # Write 1K encrypted bytes into the encrypted data buffer
    encrypted_data.write(tgcrypto.ctr256_encrypt(chunk, key, enc_iv, enc_state))

# Reset position. We need to read it now
encrypted_data.seek(0)

while True:
    chunk = encrypted_data.read(1024)

    if not chunk:
        break

    # Write 1K decrypted bytes into the decrypted data buffer
    decrypted_data.write(tgcrypto.ctr256_decrypt(chunk, key, dec_iv, dec_state))

print(data.getvalue() == decrypted_data.getvalue())  # True
```

### CBC Mode

**Note**: Data must be padded to match a multiple of the block size (16 bytes).

``` python
import os

import tgcrypto

data = os.urandom(10 * 1024 * 1024 + 7)  # 10 MB of random data + 7 bytes to show padding
key = os.urandom(32)  # Random Key

enc_iv = bytearray(os.urandom(16))  # Random IV
dec_iv = enc_iv.copy()  # Keep a copy for decryption

# Pad with zeroes: -7 % 16 = 9
data += bytes(-len(data) % 16)

cbc_encrypted = tgcrypto.cbc256_encrypt(data, key, enc_iv)
cbc_decrypted = tgcrypto.cbc256_decrypt(cbc_encrypted, key, dec_iv)

print(data == cbc_decrypted)  # True
```

## Testing

1. Clone this repository: `git clone https://github.com/pyrogram/tgcrypto`.
2. Enter the directory: `cd tgcrypto`.
3. Install `tox`: `pip3 install tox`
4. Run tests: `tox`.

## License

[LGPLv3+](COPYING.lesser) © 2017-present [Dan](https://github.com/delivrance)
