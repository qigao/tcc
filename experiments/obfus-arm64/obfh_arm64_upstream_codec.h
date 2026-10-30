/* SPDX-License-Identifier: MIT
 * Isolated AArch64 qualification of the exact upstream obfus.h packed WORD
 * format at Git blob 4c06fbc06f907afb3989e2851bed7583cfd0b6b0.
 * Pure 32-bit C11 arithmetic is intentional: these transforms are not
 * architecture-specific and should not be replaced with x86 byte sequences.
 * This file does NOT provide the original full VM or any fallback engine.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_CODEC_H
#define QIGAO_OBFH_ARM64_UPSTREAM_CODEC_H
#include <stdint.h>
#include <limits.h>
#include "obfh_arm64_poc.h"

#if UINT_MAX != UINT32_MAX
#error "Upstream obfus.h requires exactly 32-bit unsigned int."
#endif

typedef struct {
    uint32_t opcode, d, a, b, immediate;
} obfh_upstream_fields;

/* 6 permutations from OBFH_VM_FIELD_{D,A,B} in upstream region 11. */
static inline unsigned int
obfh_upstream_field_offset(uint32_t seed, unsigned int role) {
    static const unsigned char offsets[6][3] = {
        {8,10,12}, {8,12,10}, {10,12,8},
        {10,8,12}, {12,8,10}, {12,10,8}
    };
    return offsets[(seed >> 14u) % 6u][role];
}

static inline unsigned int
obfh_upstream_packed_shift(unsigned int raw_shift, uint32_t layout) {
    if (layout == 1u) return raw_shift + 10u;
    if (layout == 2u) return raw_shift - 8u;
    if (layout == 3u) return raw_shift == 8u ? 12u : raw_shift - 2u;
    return raw_shift;
}

static inline uint32_t
obfh_upstream_opcode_encode(uint32_t seed, uint32_t key, uint32_t opcode) {
    uint32_t mult = ((key >> 8u) & 255u) | 1u, salt = key & 255u;
    uint32_t value;
    switch ((seed >> 24u) % 6u) {
    case 0: value = opcode * mult + salt; break;
    case 1: value = (opcode * mult) ^ salt; break;
    case 2: value = (opcode ^ salt) * mult; break;
    case 3: value = (opcode + salt) * mult; break;
    case 4: value = opcode * mult - salt; break;
    default: value = (opcode - salt) * mult; break;
    }
    return value & 255u;
}
static inline uint32_t
obfh_upstream_opcode_decode(uint32_t seed, uint32_t key, uint32_t encoded) {
    uint32_t mult = ((key >> 8u) & 255u) | 1u, inverse = 1u;
    uint32_t salt = key & 255u, value;
    unsigned int j;
    /* This is the exact three-round odd-multiplier inverse from upstream. */
    for (j = 0u; j < 3u; ++j)
        inverse *= 2u - mult * inverse;
    switch ((seed >> 24u) % 6u) {
    case 0: value = (encoded - salt) * inverse; break;
    case 1: value = (encoded ^ salt) * inverse; break;
    case 2: value = (encoded * inverse) ^ salt; break;
    case 3: value = encoded * inverse - salt; break;
    case 4: value = (encoded + salt) * inverse; break;
    default: value = encoded * inverse + salt; break;
    }
    return value & 255u;
}

static inline unsigned int obfh_upstream_word_mode(uint32_t seed) {
    return ((seed >> 6u) & 15u) % 9u;
}
static inline uint32_t obfh_upstream_mask(uint32_t seed, uint32_t key,
                                          unsigned int pc) {
    uint32_t step = (uint32_t)pc * UINT32_C(0x9e3779b9);
    switch (obfh_upstream_word_mode(seed) % 3u) {
    case 0: return key + step;
    case 1: return key ^ step;
    default: return key - step;
    }
}
static inline uint32_t obfh_upstream_word_encode(uint32_t seed, uint32_t key,
                                                 unsigned int pc,
                                                 uint32_t rotated_word) {
    uint32_t mask = obfh_upstream_mask(seed,key,pc);
    switch (obfh_upstream_word_mode(seed) / 3u) {
    case 0: return rotated_word ^ mask;
    case 1: return rotated_word + mask;
    default: return rotated_word - mask;
    }
}
static inline uint32_t obfh_upstream_word_decode(uint32_t seed, uint32_t key,
                                                 unsigned int pc,
                                                 uint32_t encrypted_word) {
    uint32_t mask = obfh_upstream_mask(seed,key,pc);
    switch (obfh_upstream_word_mode(seed) / 3u) {
    case 0: return encrypted_word ^ mask;
    case 1: return encrypted_word - mask;
    default: return encrypted_word + mask;
    }
}
static inline uint32_t obfh_upstream_ror(uint32_t value, unsigned int distance) {
    return (value >> distance) | (value << (32u - distance));
}
static inline uint32_t obfh_upstream_rol(uint32_t value, unsigned int distance) {
    return (value << distance) | (value >> (32u - distance));
}

/* Returns 1 on a representable, exact upstream packed word; returns 0 for
 * unrepresentable register/immediate/opcode or out-of-range pc without
 * modifying caller-owned output. 0 <= pc < 32 matches upstream max program.
 */
static inline int obfh_upstream_encode(uint32_t seed, uint32_t key,
                                       unsigned int pc,
                                       const obfh_upstream_fields *fields,
                                       uint32_t *encoded) {
    uint32_t layout, op_shift, imm_shift, packed, r;
    unsigned int d_shift, a_shift, b_shift;
    if (!fields || !encoded || pc >= 32u ||
        fields->opcode > 255u || fields->d >= 4u ||
        fields->a >= 4u || fields->b >= 4u ||
        fields->immediate > 0x3ffffu)
        return 0;
    layout = (key >> 29u) & 3u;
    op_shift = layout == 1u ? 24u : layout == 2u ? 6u : 0u;
    imm_shift = layout == 1u ? 0u : 14u;
    d_shift = obfh_upstream_packed_shift(obfh_upstream_field_offset(seed,0u),layout);
    a_shift = obfh_upstream_packed_shift(obfh_upstream_field_offset(seed,1u),layout);
    b_shift = obfh_upstream_packed_shift(obfh_upstream_field_offset(seed,2u),layout);
    packed = (obfh_upstream_opcode_encode(seed,key,fields->opcode) << op_shift) |
             (fields->d << d_shift) | (fields->a << a_shift) |
             (fields->b << b_shift) | (fields->immediate << imm_shift);
    r = key % 31u + 1u;
    *encoded = obfh_upstream_word_encode(seed,key,pc,obfh_upstream_ror(packed,r));
    return 1;
}

/* All 32 bits are occupied by valid fields. Wrong keys/corrupt words are
 * NOT authenticated by this bijective codec. The caller must independently
 * validate its decoded instruction's opcode, operands and program shape.
 */
static inline int obfh_upstream_decode(uint32_t seed, uint32_t key,
                                       unsigned int pc, uint32_t encoded,
                                       obfh_upstream_fields *fields) {
    uint32_t layout, word, r, op_shift, imm_shift;
    unsigned int d_shift, a_shift, b_shift;
    if (!fields || pc >= 32u) return 0;
    layout = (key >> 29u) & 3u;
    op_shift = layout == 1u ? 24u : layout == 2u ? 6u : 0u;
    imm_shift = layout == 1u ? 0u : 14u;
    r = key % 31u + 1u;
    word = obfh_upstream_rol(obfh_upstream_word_decode(seed,key,pc,encoded),r);
    d_shift = obfh_upstream_packed_shift(obfh_upstream_field_offset(seed,0u),layout);
    a_shift = obfh_upstream_packed_shift(obfh_upstream_field_offset(seed,1u),layout);
    b_shift = obfh_upstream_packed_shift(obfh_upstream_field_offset(seed,2u),layout);
    fields->opcode = obfh_upstream_opcode_decode(seed,key,(word >> op_shift) & 255u);
    fields->d = (word >> d_shift) & 3u;
    fields->a = (word >> a_shift) & 3u;
    fields->b = (word >> b_shift) & 3u;
    fields->immediate = (word >> imm_shift) & 0x3ffffu;
    return 1;
}
#endif
