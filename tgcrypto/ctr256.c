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

void ctr256(const uint8_t in[], uint8_t out[], size_t length,
            const uint8_t key[32], const uint8_t iv[16], uint8_t *state) {
    aes256_key ctx;
    uint8_t counter[AES_BLOCK_SIZE];
    uint8_t chunk[AES_BLOCK_SIZE];
    size_t i = 0;

    aes256_key_init(&ctx, key, 0);
    memcpy(counter, iv, AES_BLOCK_SIZE);
    aes256_block_encrypt(&ctx, counter, chunk);

    /* Finish the partially-consumed block first (if any). */
    while (*state != 0 && i < length) {
        out[i] = in[i] ^ chunk[*state];
        i++;
        if (++(*state) >= AES_BLOCK_SIZE) {
            *state = 0;
            ctr_inc(counter);
            aes256_block_encrypt(&ctx, counter, chunk);
        }
    }

    /* Full blocks: XOR 16 bytes at a time using 64-bit word ops. */
    while (i + AES_BLOCK_SIZE <= length) {
        uint64_t a, b, c, d;
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
        aes256_block_encrypt(&ctx, counter, chunk);
    }

    /* Remaining tail bytes. */
    while (i < length) {
        out[i] = in[i] ^ chunk[*state];
        i++;
        (*state)++;
    }
}
