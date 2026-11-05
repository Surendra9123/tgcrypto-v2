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

Measured with GitHub Actions on standard runners, comparing the original PyPI
**tgcrypto 1.2.5** against this build (**v1.3.1**), Python 3.11. Bulk = 8 MiB buffers;
small = 64-byte payloads. Throughput in MB/s.

### macOS (Apple Silicon, arm64) — hardware AES

| Operation | original | this | speedup |
|---|---:|---:|---:|
| CBC encrypt | 153.6 | 558.8 | **3.6x** |
| CBC decrypt | 206.4 | 1391.6 | **6.7x** |
| CTR | 162.9 | 2756.2 | **16.9x** |
| IGE encrypt | 120.9 | 579.5 | **4.8x** |
| IGE decrypt | 161.6 | 403.2 | **2.5x** |
| small 64B (CTR) | 77.9 | 162.7 | **2.1x** |

### Linux (x86_64, glibc) — hardware AES-NI

| Operation | original | this | speedup |
|---|---:|---:|---:|
| CBC encrypt | 140.3 | 668.0 | **4.8x** |
| CBC decrypt | 143.3 | 2206.3 | **15.4x** |
| CTR | 108.0 | 654.9 | **6.1x** |
| IGE encrypt | 137.7 | 665.5 | **4.8x** |
| IGE decrypt | 137.7 | 664.2 | **4.8x** |
| small 64B (CTR) | 68.6 | 87.2 | **1.3x** |

### Windows (x64) — software fallback (MSVC)

MSVC does not support the function `target("aes")` attribute used to enable AES-NI
inlining, so the x86 hardware path is compiled only for GCC/Clang. Windows therefore
uses the portable software AES path; gains come from the faster mode loops and cheaper
key schedule only.

| Operation | original | this | speedup |
|---|---:|---:|---:|
| CBC encrypt | 259.2 | 276.6 | 1.1x |
| CBC decrypt | 308.1 | 314.3 | 1.0x |
| CTR | 199.2 | 298.5 | **1.5x** |
| IGE encrypt | 234.4 | 273.1 | 1.2x |
| IGE decrypt | 235.6 | 274.2 | 1.2x |
| small 64B (CTR) | 103.0 | 132.5 | 1.3x |

> **TL;DR** — With hardware AES (Apple Silicon, ARM servers, x86 GCC/Clang builds)
> bulk throughput improves roughly **2.5x–17x** depending on the mode; CTR and CBC-decrypt
> benefit most. On Windows (MSVC) the software path still gains ~1.0x–1.5x. Small-payload
> latency improves modestly across the board (the key schedule is cheaper).

> Reproduce with the **Benchmark Compare** GitHub Actions workflow (`.github/workflows/benchmark-compare.yml`).

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
