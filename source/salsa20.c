#include "salsa20.h"
#include <string.h>

#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))

#define QUARTERROUND(a, b, c, d) \
    b ^= ROTL32(a + d, 7);       \
    c ^= ROTL32(b + a, 9);       \
    d ^= ROTL32(c + b, 13);      \
    a ^= ROTL32(d + c, 18);

static uint32_t load_le32(const uint8_t *p) {
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void store_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

void salsa20_init(salsa20_ctx_t *ctx, const uint8_t key[32], const uint8_t nonce[8]) {
    static const char sigma[] = "expand 32-byte k";

    ctx->state[0]  = load_le32((const uint8_t *)sigma + 0);
    ctx->state[1]  = load_le32(key + 0);
    ctx->state[2]  = load_le32(key + 4);
    ctx->state[3]  = load_le32(key + 8);
    ctx->state[4]  = load_le32(key + 12);
    ctx->state[5]  = load_le32((const uint8_t *)sigma + 4);
    ctx->state[6]  = load_le32(nonce + 0);
    ctx->state[7]  = load_le32(nonce + 4);
    ctx->state[8]  = 0; /* Counter low */
    ctx->state[9]  = 0; /* Counter high */
    ctx->state[10] = load_le32((const uint8_t *)sigma + 8);
    ctx->state[11] = load_le32(key + 16);
    ctx->state[12] = load_le32(key + 20);
    ctx->state[13] = load_le32(key + 24);
    ctx->state[14] = load_le32(key + 28);
    ctx->state[15] = load_le32((const uint8_t *)sigma + 12);
}

static void salsa20_block(const uint32_t in[16], uint8_t out[64]) {
    uint32_t x[16];
    for (int i = 0; i < 16; ++i) {
        x[i] = in[i];
    }

    for (int i = 0; i < 10; ++i) {
        /* Column round */
        QUARTERROUND(x[0], x[4], x[8],  x[12]);
        QUARTERROUND(x[5], x[9], x[13], x[1]);
        QUARTERROUND(x[10], x[14], x[2], x[6]);
        QUARTERROUND(x[15], x[3], x[7], x[11]);
        /* Row round */
        QUARTERROUND(x[0], x[1], x[2], x[3]);
        QUARTERROUND(x[5], x[6], x[7], x[4]);
        QUARTERROUND(x[10], x[11], x[8], x[9]);
        QUARTERROUND(x[15], x[12], x[13], x[14]);
    }

    for (int i = 0; i < 16; ++i) {
        store_le32(out + (i * 4), x[i] + in[i]);
    }
}

void salsa20_crypt(salsa20_ctx_t *ctx, const uint8_t *in, uint8_t *out, size_t len) {
    uint8_t keystream[64];
    size_t offset = 0;

    while (offset < len) {
        salsa20_block(ctx->state, keystream);

        /* Increment 64-bit block counter */
        ctx->state[8]++;
        if (ctx->state[8] == 0) {
            ctx->state[9]++;
        }

        size_t chunk = (len - offset < 64) ? (len - offset) : 64;
        for (size_t i = 0; i < chunk; ++i) {
            out[offset + i] = in[offset + i] ^ keystream[i];
        }
        offset += chunk;
    }
}
