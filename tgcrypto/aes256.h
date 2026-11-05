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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef AES256_H
#define AES256_H

#define AES_BLOCK_SIZE 16
#define EXPANDED_KEY_SIZE 60
#define AES256_ROUND_KEYS (AES_BLOCK_SIZE * 15) /* 15 round keys of 16 bytes */

/* Legacy software-only API (kept for compatibility) */
void aes256_set_encryption_key(const uint8_t key[32], uint32_t expandedKey[60]);
void aes256_set_decryption_key(const uint8_t key[32], uint32_t expandedKey[60]);
void aes256_encrypt(const uint8_t in[16], uint8_t out[16], const uint32_t expandedKey[60]);
void aes256_decrypt(const uint8_t in[16], uint8_t out[16], const uint32_t expandedKey[60]);

/* Dispatched AES context: uses hardware AES when available, else software */
typedef struct {
    int      hw;                     /* 1 if hardware path is active */
    uint32_t ek[EXPANDED_KEY_SIZE];  /* software encryption key schedule */
    uint32_t dk[EXPANDED_KEY_SIZE];  /* software decryption key schedule */
    uint8_t  hw_ek[AES256_ROUND_KEYS]; /* hardware encryption round keys (AES byte order) */
    uint8_t  hw_dk[AES256_ROUND_KEYS]; /* hardware decryption round keys */
} aes256_key;

void aes256_key_init(aes256_key *k, const uint8_t key[32], int need_decrypt);
void aes256_block_encrypt(const aes256_key *k, const uint8_t in[16], uint8_t out[16]);
void aes256_block_decrypt(const aes256_key *k, const uint8_t in[16], uint8_t out[16]);

/* Hardware backend (implemented in aes256_hw.c; stub-fallback on unsupported CPUs) */
int  aes256_hw_detect(void);
void aes256_hw_expand(const uint8_t key[32], uint8_t hw_ek[AES256_ROUND_KEYS], uint8_t hw_dk[AES256_ROUND_KEYS]);
void aes256_hw_encrypt_block(const uint8_t hw_ek[AES256_ROUND_KEYS], const uint8_t in[16], uint8_t out[16]);
void aes256_hw_decrypt_block(const uint8_t hw_dk[AES256_ROUND_KEYS], const uint8_t in[16], uint8_t out[16]);

#endif  // AES256_H
