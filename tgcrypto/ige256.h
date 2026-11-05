#include "aes256.h"

#ifndef IGE256_H
#define IGE256_H

void ige256(const uint8_t in[], uint8_t out[], size_t length,
            const uint8_t key[32], const uint8_t iv[32], uint8_t encrypt);

#endif  // IGE256_H
