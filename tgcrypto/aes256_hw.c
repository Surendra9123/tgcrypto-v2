/*
 * Pyrogram - Telegram MTProto API Client Library for Python
 * Copyright (C) 2017-present Dan <https://github.com/delivrance>
 *
 * This file is part of Pyrogram.
 *
 * Pyrogram is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Pyrogram is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Pyrogram.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Hardware AES-256 backend with runtime dispatch.
 *
 * Supported accelerators:
 *   - ARMv8 Crypto Extensions (AESE/AESD/AESMC/AESIMC) : aarch64
 *   - x86/x86_64 AES-NI (AESENC/AESDEC/AESIMC)       : via target("aes")
 *
 * On any other CPU / when the feature is absent at runtime, aes256_hw_detect()
 * returns 0 and the caller uses the portable table-based software path.
 * The key schedule below is fully portable and produces round keys in the
 * natural AES byte order understood by both instruction sets.
 */

#include "aes256.h"

#if defined(__aarch64__) && defined(__ARM_FEATURE_CRYPTO)
#  define HAVE_ARM_AES 1
#  include <arm_neon.h>
#elif (defined(__x86_64__) || defined(__i386__)) && (defined(__clang__) || defined(__GNUC__))
/* x86 AES-NI via target("aes") function attribute (clang / GCC only).
 * MSVC and other compilers fall back to the portable software path. */
#  define HAVE_X86_AES 1
#  include <wmmintrin.h>
#  include <emmintrin.h>
#endif

/* ---- Portable AES-256 round key schedule (natural byte order) ---------- */

static const uint8_t AES_SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t AES_RCON[11] = {
    0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36
};

/* Build 15 round keys (240 bytes) for AES-256 in natural byte order. */
static void aes256_expand_bytes(const uint8_t key[32], uint8_t rk[15 * 16]) {
    uint8_t t[4];
    int i;

    memcpy(rk, key, 32);

    for (i = 8; i < 60; i++) {
        memcpy(t, rk + (i - 1) * 4, 4);

        if (i % 8 == 0) {
            uint8_t tmp = t[0];
            t[0] = (uint8_t)(AES_SBOX[t[1]] ^ AES_RCON[i / 8]);
            t[1] = AES_SBOX[t[2]];
            t[2] = AES_SBOX[t[3]];
            t[3] = AES_SBOX[tmp];
        } else if (i % 8 == 4) {
            t[0] = AES_SBOX[t[0]];
            t[1] = AES_SBOX[t[1]];
            t[2] = AES_SBOX[t[2]];
            t[3] = AES_SBOX[t[3]];
        }

        rk[i * 4 + 0] = rk[(i - 8) * 4 + 0] ^ t[0];
        rk[i * 4 + 1] = rk[(i - 8) * 4 + 1] ^ t[1];
        rk[i * 4 + 2] = rk[(i - 8) * 4 + 2] ^ t[2];
        rk[i * 4 + 3] = rk[(i - 8) * 4 + 3] ^ t[3];
    }
}

/* GF(2^8) multiply-by-2 helpers for InvMixColumns */
#define XT(a) ((uint8_t)(((a) << 1) ^ (((a) & 0x80) ? 0x1b : 0x00)))

/* Apply InvMixColumns to a single 16-byte round key (for equivalent inverse cipher). */
static void aes256_invmix_column_key(const uint8_t in[16], uint8_t out[16]) {
    int c;
    for (c = 0; c < 4; c++) {
        uint8_t a0 = in[c * 4 + 0], a1 = in[c * 4 + 1], a2 = in[c * 4 + 2], a3 = in[c * 4 + 3];
        uint8_t t = a0 ^ a1 ^ a2 ^ a3;
        out[c * 4 + 0] = a0 ^ t ^ XT(a0 ^ a1);
        out[c * 4 + 1] = a1 ^ t ^ XT(a1 ^ a2);
        out[c * 4 + 2] = a2 ^ t ^ XT(a2 ^ a3);
        out[c * 4 + 3] = a3 ^ t ^ XT(a3 ^ a0);
    }
}

/* ---- Runtime detection ------------------------------------------------- */

#if defined(HAVE_ARM_AES)
#  if defined(__APPLE__)
     static int hw_probe(void) { return 1; }              /* Apple arm64: crypto baseline */
#  elif defined(__linux__)
#    include <sys/auxv.h>
#    ifndef HWCAP_AES
#      define HWCAP_AES (1 << 3)
#    endif
     static int hw_probe(void) { return (getauxval(AT_HWCAP) & HWCAP_AES) != 0; }
#  else
     static int hw_probe(void) { return 1; }              /* assume presence */
#  endif
#elif defined(HAVE_X86_AES)
#  if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
     static int hw_probe(void) { __builtin_cpu_init(); return __builtin_cpu_supports("aes"); }
#  else
     static int hw_probe(void) { return 0; }
#  endif
#else
   static int hw_probe(void) { return 0; }
#endif

static int hw_initialized = 0;
static int hw_available   = 0;

int aes256_hw_detect(void) {
    if (!hw_initialized) {
        const char *force = getenv("TGCRYPTO_FORCE_SOFTWARE");
        if (force && force[0] && force[0] != '0')
            hw_available = 0;   /* allow forcing the portable software path */
        else
            hw_available = hw_probe();
        hw_initialized = 1;
    }
    return hw_available;
}

/* ---- Key expansion (portable) ------------------------------------------ */

void aes256_hw_expand(const uint8_t key[32], uint8_t hw_ek[240], uint8_t hw_dk[240]) {
    int build_dk = (hw_dk != NULL);
    int i;

    aes256_expand_bytes(key, hw_ek);

    if (!build_dk) return;
#if defined(HAVE_ARM_AES)
    /* ARM: decryption recipe consumes the *plain* encryption round keys
     * (see aes256_hw_decrypt_block), so just mirror them. */
    memcpy(hw_dk, hw_ek, 240);
#else
    /* x86 AES-NI equivalent inverse cipher:
     *   dk[0]  = ek[14]
     *   dk[i]  = InvMixColumns(ek[14-i])  for i = 1..13
     *   dk[14] = ek[0] */
    memcpy(hw_dk, hw_ek + 14 * 16, 16);
    for (i = 1; i <= 13; i++)
        aes256_invmix_column_key(hw_ek + (14 - i) * 16, hw_dk + i * 16);
    memcpy(hw_dk + 14 * 16, hw_ek, 16);
#endif
    (void) i;
}

/* ---- Per-architecture single-block primitives -------------------------- */

#if defined(HAVE_ARM_AES)

void aes256_hw_encrypt_block(const uint8_t rk[240], const uint8_t in[16], uint8_t out[16]) {
    uint8x16_t s = vaeseq_u8(vld1q_u8(in), vld1q_u8(rk));
    for (int i = 1; i < 14; i++) {
        s = vaesmcq_u8(s);
        s = vaeseq_u8(s, vld1q_u8(rk + i * 16));
    }
    s = veorq_u8(s, vld1q_u8(rk + 14 * 16));
    vst1q_u8(out, s);
}

void aes256_hw_decrypt_block(const uint8_t rk[240], const uint8_t in[16], uint8_t out[16]) {
    const uint8x16_t zero = vdupq_n_u8(0);
    uint8x16_t s = veorq_u8(vld1q_u8(in), vld1q_u8(rk + 14 * 16));
    for (int i = 13; i >= 1; i--) {
        s = vaesdq_u8(s, zero);
        s = veorq_u8(s, vld1q_u8(rk + i * 16));
        s = vaesimcq_u8(s);
    }
    s = veorq_u8(vaesdq_u8(s, zero), vld1q_u8(rk));
    vst1q_u8(out, s);
}

#elif defined(HAVE_X86_AES)

__attribute__((target("aes")))
void aes256_hw_encrypt_block(const uint8_t rk[240], const uint8_t in[16], uint8_t out[16]) {
    __m128i s = _mm_loadu_si128((const __m128i *) in);
    s = _mm_xor_si128(s, _mm_loadu_si128((const __m128i *) rk));
    for (int i = 1; i < 14; i++)
        s = _mm_aesenc_si128(s, _mm_loadu_si128((const __m128i *) (rk + i * 16)));
    s = _mm_aesenclast_si128(s, _mm_loadu_si128((const __m128i *) (rk + 14 * 16)));
    _mm_storeu_si128((__m128i *) out, s);
}

__attribute__((target("aes")))
void aes256_hw_decrypt_block(const uint8_t rk[240], const uint8_t in[16], uint8_t out[16]) {
    __m128i s = _mm_loadu_si128((const __m128i *) in);
    s = _mm_xor_si128(s, _mm_loadu_si128((const __m128i *) rk));
    for (int i = 1; i < 14; i++)
        s = _mm_aesdec_si128(s, _mm_loadu_si128((const __m128i *) (rk + i * 16)));
    s = _mm_aesdeclast_si128(s, _mm_loadu_si128((const __m128i *) (rk + 14 * 16)));
    _mm_storeu_si128((__m128i *) out, s);
}

#else

/* No hardware backend compiled in: these are never called because
 * aes256_hw_detect() returns 0. Provided to satisfy the linker. */
void aes256_hw_encrypt_block(const uint8_t rk[240], const uint8_t in[16], uint8_t out[16]) {
    (void) rk; memcpy(out, in, 16);
}
void aes256_hw_decrypt_block(const uint8_t rk[240], const uint8_t in[16], uint8_t out[16]) {
    (void) rk; memcpy(out, in, 16);
}

#endif
