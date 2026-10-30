/* SPDX-License-Identifier: MIT
 * Original obfus.h region-09 OBFH_VM_VALUE byte transport, isolated POC.
 * Baseline: DosX-dev/obfus.h Git blob 4c06fbc06f907afb3989e2851bed7583cfd0b6b0.
 * NO float arithmetic or silent double fallback: all long double object
 * bytes, including padding, signed zero, NaN payload and nonce, are carried.
 *
 * The original int/double/condition proxy chain is identity for integer
 * salts 16..48, which are exactly SALT_NUM1/SALT_NUM2's source range.
 * Seed is an explicit test input to cover all source-selected modes.
 * No production VM, authentication or cryptographic security is claimed.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_VALUE_H
#define QIGAO_OBFH_ARM64_UPSTREAM_VALUE_H
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include "obfh_arm64_poc.h"

#if UINT_MAX != UINT32_MAX || CHAR_BIT != 8
#error "Original obfus.h operand transport needs 32-bit unsigned int and 8-bit bytes."
#endif

typedef struct {
    unsigned char bytes[sizeof(long double)];
    unsigned char floating;
    uint32_t nonce;
} obfh_arm64_upstream_value;

enum obfh_arm64_upstream_value_status {
    OBFH_UPVALUE_OK = 0,
    OBFH_UPVALUE_INVALID_ARGS = 1
};

/* Source exact xorshift triple: mode bits 21/22 swap direction and distance.
 * Each shift is strictly between 1 and 31, on an unsigned 32-bit word.
 */
static inline uint32_t obfh_upvalue_step(uint32_t state, uint32_t seed) {
    unsigned int first = ((seed >> 22u) & 1u) ? 5u : 13u;
    unsigned int last = ((seed >> 22u) & 1u) ? 13u : 5u;
    if ((seed >> 21u) & 1u) {
        state ^= state >> first;
        state ^= state << 17u;
        state ^= state >> last;
    } else {
        state ^= state << first;
        state ^= state >> 17u;
        state ^= state << last;
    }
    return state;
}

/* Fail-closed argument admission and transactional caller-owned output.
 * The upstream first argument is a long double object; explicitly accepting
 * its raw bytes avoids undefined casts from arbitrary NaN byte fixtures.
 * floating_nonce has original packing: bit0 floating, upper31 nonce input.
 */
static inline enum obfh_arm64_upstream_value_status
obfh_arm64_upvalue_encode(const unsigned char *raw, size_t size,
                          uint32_t seed, int salt, uint32_t floating_nonce,
                          obfh_arm64_upstream_value *output) {
    obfh_arm64_upstream_value packet;
    uint32_t state;
    size_t i, offset;
    if (!raw || !output || size != sizeof(long double) ||
        salt < 16 || salt > 48) return OBFH_UPVALUE_INVALID_ARGS;

    packet.nonce = (floating_nonce >> 1u) ^ seed;
    packet.floating = (unsigned char)(floating_nonce & 1u);
    state = packet.nonce ^ ((uint32_t)salt * UINT32_C(2246822519)) ^
            UINT32_C(0x9e3779b9);
    offset = packet.nonce % sizeof(long double);
    for (i=0u; i<sizeof(long double); ++i) {
        unsigned int rotate;
        unsigned char value, add, mask;
        state = obfh_upvalue_step(state, seed);
        rotate = (state >> 24u) & 7u;
        value = (unsigned char)(((unsigned int)raw[i] << rotate) |
                                ((unsigned int)raw[i] >> (8u-rotate)));
        add = (unsigned char)(state >> 8u);
        mask = (unsigned char)(state ^ (state >> 16u));
        if ((seed >> 10u) & 1u)
            packet.bytes[(i+offset)%sizeof(long double)] =
                (unsigned char)(((unsigned int)(value ^ mask) + add) & 255u);
        else
            packet.bytes[(i+offset)%sizeof(long double)] =
                (unsigned char)((((unsigned int)value + add) & 255u) ^ mask);
    }
    *output = packet;
    return OBFH_UPVALUE_OK;
}

static inline enum obfh_arm64_upstream_value_status
obfh_arm64_upvalue_decode(const obfh_arm64_upstream_value *packet,
                          uint32_t seed, int salt, unsigned char *output,
                          size_t size) {
    uint32_t state;
    unsigned char decoded[sizeof(long double)];
    size_t i, offset;
    if (!packet || !output || size != sizeof(long double) ||
        salt < 16 || salt > 48 || packet->floating > 1u)
        return OBFH_UPVALUE_INVALID_ARGS;
    state = packet->nonce ^ ((uint32_t)salt * UINT32_C(2246822519)) ^
            UINT32_C(0x9e3779b9);
    offset = packet->nonce % sizeof(long double);
    for (i=0u; i<sizeof(long double); ++i) {
        unsigned int rotate;
        unsigned char value, add, mask;
        state = obfh_upvalue_step(state, seed);
        rotate = (state >> 24u) & 7u;
        value = packet->bytes[(i+offset)%sizeof(long double)];
        add = (unsigned char)(state >> 8u);
        mask = (unsigned char)(state ^ (state >> 16u));
        if ((seed >> 10u) & 1u)
            value = (unsigned char)((((unsigned int)value - add) & 255u) ^ mask);
        else
            value = (unsigned char)((((unsigned int)value ^ mask) - add) & 255u);
        decoded[i] = (unsigned char)(((unsigned int)value >> rotate) |
                                    ((unsigned int)value << (8u-rotate)));
    }
    for (i=0u; i<sizeof(long double); ++i) output[i] = decoded[i];
    return OBFH_UPVALUE_OK;
}
#endif
