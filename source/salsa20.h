#ifndef SALSA20_H
#define SALSA20_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t state[16];
} salsa20_ctx_t;

/**
 * Initialize Salsa20 state with a 256-bit (32-byte) key and 64-bit (8-byte) nonce.
 */
void salsa20_init(salsa20_ctx_t *ctx, const uint8_t key[32], const uint8_t nonce[8]);

/**
 * Decrypt/Encrypt arbitrary length buffer using Salsa20 stream cipher.
 */
void salsa20_crypt(salsa20_ctx_t *ctx, const uint8_t *in, uint8_t *out, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SALSA20_H */
