#include "aes256.h"

#ifndef CTR256_H
#define CTR256_H

void ctr256(const uint8_t in[], uint8_t out[], size_t length,
            const uint8_t key[32], const uint8_t iv[16], uint8_t *state);

#endif  // CTR256_H
