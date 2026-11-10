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

#include "ctr256.h"

/* Increment a 16-byte big-endian counter in place. */
static inline void ctr_inc(uint8_t counter[AES_BLOCK_SIZE]) {
    size_t k = AES_BLOCK_SIZE;
    while (k--)
        if (++counter[k])
            break;
}

#define CTR_BATCH 16

void ctr256(const uint8_t in[], uint8_t out[], size_t length,
            const uint8_t key[32], const uint8_t iv[16], uint8_t *state) {
    aes256_key ctx;
    uint8_t counter[AES_BLOCK_SIZE];
    uint8_t chunk[AES_BLOCK_SIZE];
    size_t i = 0;

    aes256_key_init(&ctx, key, 0);
    memcpy(counter, iv, AES_BLOCK_SIZE);

    /* Finish a partially-consumed first block (stream continuation). */
    if (*state != 0) {
        aes256_block_encrypt(&ctx, counter, chunk);
        while (*state < AES_BLOCK_SIZE && i < length) {
            out[i] = in[i] ^ chunk[*state];
            i++;
            (*state)++;
        }
        if (*state >= AES_BLOCK_SIZE) {
            *state = 0;
            ctr_inc(counter);
        }
    }

    /* Hardware fast path: encrypt a batch of counter blocks in parallel,
     * then XOR the whole group (interleaved AES hides instruction latency). */
    if (ctx.hw) {
        while (i + CTR_BATCH * AES_BLOCK_SIZE <= length) {
            uint8_t ctrs[CTR_BATCH * AES_BLOCK_SIZE];
            uint8_t ks[CTR_BATCH * AES_BLOCK_SIZE];
            size_t b, j;
            for (b = 0; b < CTR_BATCH; b++) {
                memcpy(ctrs + b * AES_BLOCK_SIZE, counter, AES_BLOCK_SIZE);
                ctr_inc(counter);
            }
            aes256_hw_encrypt_blocks(ctx.hw_ek, ctrs, ks, CTR_BATCH);
            for (j = 0; j < CTR_BATCH * AES_BLOCK_SIZE; j += 8) {
                uint64_t a, c;
                memcpy(&a, in + i + j, 8);
                memcpy(&c, ks + j, 8);
                a ^= c;
                memcpy(out + i + j, &a, 8);
            }
            i += CTR_BATCH * AES_BLOCK_SIZE;
        }
    }

    /* Remaining full blocks (hardware single-block or software). */
    while (i + AES_BLOCK_SIZE <= length) {
        uint64_t a, b, c, d;
        aes256_block_encrypt(&ctx, counter, chunk);
        memcpy(&a, in + i, 8);
        memcpy(&b, in + i + 8, 8);
        memcpy(&c, chunk, 8);
        memcpy(&d, chunk + 8, 8);
        a ^= c;
        b ^= d;
        memcpy(out + i, &a, 8);
        memcpy(out + i + 8, &b, 8);
        i += AES_BLOCK_SIZE;
        ctr_inc(counter);
    }

    /* Tail bytes. */
    if (i < length) {
        aes256_block_encrypt(&ctx, counter, chunk);
        while (i < length) {
            out[i] = in[i] ^ chunk[*state];
            i++;
            (*state)++;
        }
    }
}
