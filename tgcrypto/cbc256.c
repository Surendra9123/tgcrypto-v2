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

#include "cbc256.h"

static inline void xor16(uint8_t dst[16], const uint8_t a[16], const uint8_t b[16]) {
    uint64_t x, y, p, q;
    memcpy(&x, a, 8); memcpy(&y, a + 8, 8);
    memcpy(&p, b, 8); memcpy(&q, b + 8, 8);
    x ^= p; y ^= q;
    memcpy(dst, &x, 8); memcpy(dst + 8, &y, 8);
}

#define CBC_BATCH 8

void cbc256(const uint8_t in[], uint8_t out[], size_t length,
            const uint8_t key[32], const uint8_t iv[16], uint8_t encrypt) {
    aes256_key ctx;
    uint8_t chain[AES_BLOCK_SIZE];
    uint8_t nextIv[AES_BLOCK_SIZE];
    size_t i;

    aes256_key_init(&ctx, key, encrypt ? 0 : 1);
    memcpy(chain, iv, AES_BLOCK_SIZE);

    if (encrypt) {
        /* CBC encryption is inherently serial. */
        for (i = 0; i < length; i += AES_BLOCK_SIZE) {
            xor16(&out[i], &in[i], chain);
            aes256_block_encrypt(&ctx, &out[i], &out[i]);
            memcpy(chain, &out[i], AES_BLOCK_SIZE);
        }
    } else {
        i = 0;
        /* Hardware fast path: decrypt a batch in parallel, then apply chaining. */
        if (ctx.hw) {
            while (i + CBC_BATCH * AES_BLOCK_SIZE <= length) {
                uint8_t dec[CBC_BATCH * AES_BLOCK_SIZE];
                size_t blk;
                aes256_hw_decrypt_blocks(ctx.hw_dk, in + i, dec, CBC_BATCH);
                for (blk = 0; blk < CBC_BATCH; blk++) {
                    const uint8_t *ct = in + i + blk * AES_BLOCK_SIZE;
                    uint8_t *o = out + i + blk * AES_BLOCK_SIZE;
                    xor16(o, dec + blk * AES_BLOCK_SIZE, chain);
                    memcpy(chain, ct, AES_BLOCK_SIZE);
                }
                i += CBC_BATCH * AES_BLOCK_SIZE;
            }
        }
        for (; i < length; i += AES_BLOCK_SIZE) {
            memcpy(nextIv, &in[i], AES_BLOCK_SIZE);
            aes256_block_decrypt(&ctx, &in[i], &out[i]);
            xor16(&out[i], &out[i], chain);
            memcpy(chain, nextIv, AES_BLOCK_SIZE);
        }
    }
}
