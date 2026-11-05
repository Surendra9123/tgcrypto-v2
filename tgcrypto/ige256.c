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

#include "ige256.h"

static inline void xor16(uint8_t dst[16], const uint8_t a[16], const uint8_t b[16]) {
    uint64_t x, y, p, q;
    memcpy(&x, a, 8); memcpy(&y, a + 8, 8);
    memcpy(&p, b, 8); memcpy(&q, b + 8, 8);
    x ^= p; y ^= q;
    memcpy(dst, &x, 8); memcpy(dst + 8, &y, 8);
}

void ige256(const uint8_t in[], uint8_t out[], size_t length,
            const uint8_t key[32], const uint8_t iv[32], uint8_t encrypt) {
    aes256_key ctx;
    uint8_t iv1[AES_BLOCK_SIZE], iv2[AES_BLOCK_SIZE];
    uint8_t chunk[AES_BLOCK_SIZE], buffer[AES_BLOCK_SIZE];
    size_t i;

    aes256_key_init(&ctx, key, encrypt ? 0 : 1);

    if (encrypt) {
        memcpy(iv1, iv, AES_BLOCK_SIZE);
        memcpy(iv2, iv + AES_BLOCK_SIZE, AES_BLOCK_SIZE);
    } else {
        memcpy(iv2, iv, AES_BLOCK_SIZE);
        memcpy(iv1, iv + AES_BLOCK_SIZE, AES_BLOCK_SIZE);
    }

    for (i = 0; i < length; i += AES_BLOCK_SIZE) {
        memcpy(chunk, &in[i], AES_BLOCK_SIZE);

        xor16(buffer, &in[i], iv1);

        if (encrypt)
            aes256_block_encrypt(&ctx, buffer, &out[i]);
        else
            aes256_block_decrypt(&ctx, buffer, &out[i]);

        xor16(&out[i], &out[i], iv2);

        memcpy(iv1, &out[i], AES_BLOCK_SIZE);
        memcpy(iv2, chunk, AES_BLOCK_SIZE);
    }
}
