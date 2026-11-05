#include "aes256.h"

#ifndef CBC256_H
#define CBC256_H

void cbc256(const uint8_t in[], uint8_t out[], size_t length,
            const uint8_t key[32], const uint8_t iv[16], uint8_t encrypt);

#endif  // CBC256_H
