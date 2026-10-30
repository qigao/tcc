/* SPDX-License-Identifier: MIT
 * Differential qualification of exact upstream obfus.h WORD encoding.
 * Original source: DosX-dev/obfus.h/include/obfus.h blob
 * 4c06fbc06f907afb3989e2851bed7583cfd0b6b0, regions 11 and 14.
 *
 * This oracle independently spells the source's OBFH_V_PACK transformation
 * instead of using the production packed-field shifts. A hard-coded FNV64
 * digest for all 10,368 fixture words was calculated separately from the
 * upstream formulas before the C implementation. Do not regenerate it at
 * runtime; incorrect implementations must fail the native CI.
 */
#include <stdint.h>
#include "obfh_arm64_upstream_codec.h"

static uint32_t reference_opcode(uint32_t seed, uint32_t key, uint32_t op) {
    uint32_t multiplier = ((key >> 8u) & 255u) | 1u, salt = key & 255u;
    switch ((seed >> 24u) % 6u) {
    case 0: return (op * multiplier + salt) & 255u;
    case 1: return ((op * multiplier) ^ salt) & 255u;
    case 2: return ((op ^ salt) * multiplier) & 255u;
    case 3: return ((op + salt) * multiplier) & 255u;
    case 4: return (op * multiplier - salt) & 255u;
    default: return ((op - salt) * multiplier) & 255u;
    }
}

/* Upstream OBFH_V_INSTRUCTION, OBFH_V_PACK and OBFH_V_ENCODE_RAW
 * expressed literally, without sharing the new codec's shift calculation. */
static uint32_t reference_word(uint32_t seed, uint32_t key, unsigned int pc,
                               obfh_upstream_fields f) {
    uint32_t raw, rotated, mask, delta = (uint32_t)pc * UINT32_C(0x9e3779b9);
    unsigned int rotate = (unsigned int)(key % 31u) + 1u;
    unsigned int word_mode = ((seed >> 6u) & 15u) % 9u;
    uint32_t d = f.d, a = f.a, b = f.b;
    switch ((seed >> 14u) % 6u) {
    case 0: raw = (d<<8u) | (a<<10u) | (b<<12u); break;
    case 1: raw = (d<<8u) | (a<<12u) | (b<<10u); break;
    case 2: raw = (d<<10u) | (a<<12u) | (b<<8u); break;
    case 3: raw = (d<<10u) | (a<<8u) | (b<<12u); break;
    case 4: raw = (d<<12u) | (a<<8u) | (b<<10u); break;
    default: raw = (d<<12u) | (a<<10u) | (b<<8u); break;
    }
    raw |= reference_opcode(seed,key,f.opcode);
    raw |= f.immediate << 14u;
    switch ((key >> 29u) & 3u) {
    case 1:
        raw = ((raw & 255u) << 24u) |
              (((raw >> 8u) & 3u) << 18u) |
              (((raw >> 10u) & 3u) << 20u) |
              (((raw >> 12u) & 3u) << 22u) | (raw >> 14u);
        break;
    case 2:
        raw = ((raw & 255u) << 6u) | ((raw >> 8u) & 3u) |
              (((raw >> 10u) & 3u) << 2u) |
              (((raw >> 12u) & 3u) << 4u) | (raw & UINT32_C(0xffffc000));
        break;
    case 3:
        raw = (raw & 255u) | (((raw >> 8u) & 3u) << 12u) |
              (((raw >> 10u) & 3u) << 8u) |
              (((raw >> 12u) & 3u) << 10u) | (raw & UINT32_C(0xffffc000));
        break;
    default: break;
    }
    rotated = (raw >> rotate) | (raw << (32u - rotate));
    switch (word_mode % 3u) {
    case 0: mask = key + delta; break;
    case 1: mask = key ^ delta; break;
    default: mask = key - delta; break;
    }
    switch (word_mode / 3u) {
    case 0: return rotated ^ mask;
    case 1: return rotated + mask;
    default: return rotated - mask;
    }
}

static int full_cross_product(void) {
    static const uint32_t opcodes[8] = {
        0u, 1u, 0x31u, 0x80u, 0xffu, 9u, 0xe1u, 26u
    };
    static const uint32_t imms[8] = {
        0u, 1u, 0x3ffffu, 0x2aaaau, 0x15555u, 0x01fffu, 0x12345u, 0x3ffeeu
    };
    uint64_t digest = UINT64_C(0xcbf29ce484222325);
    unsigned int opmode, wordmode, perm, layout, i, k, checked=0u;
    for (opmode=0u;opmode<6u;++opmode)
        for (wordmode=0u;wordmode<9u;++wordmode)
            for (perm=0u;perm<6u;++perm)
                for (layout=0u;layout<4u;++layout) {
                    uint32_t subgroup=(perm+6u-(opmode*1024u)%6u)%6u;
                    uint32_t seed=(opmode<<24u)|(subgroup<<14u)|
                                  (wordmode<<6u)|21u;
                    uint32_t key=(layout<<29u) |
                        (((opmode+1u)*0x12345u+(wordmode+3u)*0x5b1u+
                           perm*0x7bdu)&0x1fffffffu);
                    if ((seed>>24u)%6u!=opmode ||
                        ((seed>>6u)&15u)%9u!=wordmode ||
                        (seed>>14u)%6u!=perm || ((key>>29u)&3u)!=layout)
                        return 11;
                    for (i=0u;i<8u;++i) {
                        obfh_upstream_fields input, decoded;
                        uint32_t encoded=0xfeedc0deu, reference;
                        unsigned int pc=(i*7u+perm)&31u;
                        input.opcode=opcodes[i];
                        input.d=(i^opmode)&3u;
                        input.a=(i+perm)&3u;
                        input.b=(i+wordmode)&3u;
                        input.immediate=imms[i];
                        reference=reference_word(seed,key,pc,input);
                        if (!obfh_upstream_encode(seed,key,pc,&input,&encoded) ||
                            encoded!=reference ||
                            !obfh_upstream_decode(seed,key,pc,encoded,&decoded) ||
                            decoded.opcode!=input.opcode || decoded.d!=input.d ||
                            decoded.a!=input.a || decoded.b!=input.b ||
                            decoded.immediate!=input.immediate)
                            return 12;
                        for (k=0u;k<4u;++k) {
                            digest ^= (encoded>>(8u*k))&255u;
                            digest *= UINT64_C(0x100000001b3);
                        }
                        ++checked;
                    }
                }
    /* Independent golden receipt from the original source expressions,
     * all 6 opcode × 9 word × 6 field × 4 layout × 8 operand vectors. */
    if (checked!=10368u || digest!=UINT64_C(0xb19831d024873da0))
        return 13;
    return 0;
}

/* Make the actual compiled build seed observable to the native executable;
 * the encoding seed is not a secret and does not provide authentication. */
static volatile uint32_t compiled_build_seed = (uint32_t)OBFH_SEED;

static int negative_contract(void) {
    obfh_upstream_fields f={18u,2u,0u,1u,0x3ffffu}, decoded;
    uint32_t result=0xcafebabeu, encoded;
    const uint32_t seed=compiled_build_seed;
    const uint32_t key=seed ^ UINT32_C(0x3b87a2d1);
#define BAD_ENCODE(expected) do { \
    result=0xcafebabeu; \
    if (obfh_upstream_encode(seed,key,7u,&f,&result) || \
        result!=0xcafebabeu) return (expected); \
} while(0)
    f.opcode=256u; BAD_ENCODE(21); f.opcode=18u;
    f.d=4u; BAD_ENCODE(22); f.d=2u;
    f.a=4u; BAD_ENCODE(23); f.a=0u;
    f.b=4u; BAD_ENCODE(24); f.b=1u;
    f.immediate=0x40000u; BAD_ENCODE(25); f.immediate=0x3ffffu;
#undef BAD_ENCODE
    if (obfh_upstream_encode(seed,key,32u,&f,&result) ||
        result!=0xcafebabeu) return 26;
    if (obfh_upstream_encode(seed,key,0u,(const obfh_upstream_fields *)0,
                             &result) || result!=0xcafebabeu) return 27;
    if (obfh_upstream_encode(seed,key,0u,&f,(uint32_t *)0)) return 28;
    if (obfh_upstream_decode(seed,key,32u,0u,&decoded)) return 29;
    if (obfh_upstream_decode(seed,key,0u,0u,(obfh_upstream_fields *)0))
        return 30;
    /* The compiled profile must affect a real call. Wrong-key decoding
     * is not authenticated; no false claim of generic corruption rejection.
     */
    if (!obfh_upstream_encode(seed,key,7u,&f,&encoded) ||
        !obfh_upstream_decode(seed,key,7u,encoded,&decoded) ||
        decoded.opcode!=f.opcode || decoded.immediate!=f.immediate)
        return 31;
    return 0;
}

int main(void) {
    int status=full_cross_product();
    if (status) return status;
    return negative_contract();
}
