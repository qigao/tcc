/* SPDX-License-Identifier: MIT
 * Exact region-11 VM register/flag/control BYTE transport experiment.
 * Baseline: DosX-dev/obfus.h Git blob 4c06fbc06f907afb3989e2851bed7583cfd0b6b0.
 * The *semantic* four-register state is ported; physical OBFH_V_CONTEXT
 * field-order permutations and full original interpreter are not.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_REGISTERS_H
#define QIGAO_OBFH_ARM64_UPSTREAM_REGISTERS_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "obfh_arm64_upstream_value.h"

#define OBFH_UPREG_COUNT 4u
#define OBFH_UPREG_CONTROL_FLAG UINT32_C(32)

typedef struct {
    unsigned char bytes[sizeof(long double)];
    uint32_t mask;
} obfh_upreg_slot;
typedef struct {
    obfh_upreg_slot registers[OBFH_UPREG_COUNT];
    uint32_t controls[OBFH_UPREG_COUNT];
    uint32_t flags, flag_key, key, seed, steps, initialized, flags_ready;
} obfh_upreg_state;
enum obfh_upreg_status {
    OBFH_UPREG_OK = 0,
    OBFH_UPREG_INVALID_ARGS = 1,
    OBFH_UPREG_UNINITIALIZED = 2
};

/* Upstream OBFH_MIX_FIELDS(obfh_v_helper_draw, seed ^ 0x6d2b79f5):
 * MIX_A then MIX_B on uint32, with source-picked shift/multiply direction.
 */
static inline uint32_t obfh_upreg_draw(uint32_t seed) {
    uint32_t v = seed ^ UINT32_C(0x6d2b79f5);
    v = (v ^ (v >> 16u)) * UINT32_C(2246822507);
    return (v ^ (v >> 13u)) * UINT32_C(3266489909);
}
static inline uint32_t obfh_upreg_mix(uint32_t seed, uint32_t value) {
    uint32_t draw=obfh_upreg_draw(seed), mult;
    unsigned int sa=11u+(draw&7u), sb=12u+((draw>>3u)&7u);
    if (draw & 0x40u) value ^= value << sa;
    else value ^= value >> sa;
    if (draw % 3u == 0u) mult=UINT32_C(2246822507);
    else if (draw % 3u == 1u) mult=UINT32_C(3266489909);
    else mult=UINT32_C(2221713035);
    value *= mult;
    if ((draw & 0xc0u)==0x80u) value ^= value << sb;
    else value ^= value >> sb;
    return value;
}
static inline uint32_t obfh_upreg_control_mask(const obfh_upreg_state *c,
                                                 unsigned int r) {
    uint32_t delta = (uint32_t)r * UINT32_C(3266489917);
    uint32_t draw = obfh_upreg_draw(c->seed);
    return obfh_upreg_mix(c->seed,
                           (draw & 0x200u) ? (c->key ^ delta)
                                           : (c->key + delta));
}
static inline enum obfh_upreg_status obfh_upreg_control_write(
        obfh_upreg_state *c, unsigned int r, uint32_t value) {
    if (!c || r>=OBFH_UPREG_COUNT) return OBFH_UPREG_INVALID_ARGS;
    c->controls[r] = value ^ obfh_upreg_control_mask(c,r);
    return OBFH_UPREG_OK;
}
static inline enum obfh_upreg_status obfh_upreg_control_read(
        const obfh_upreg_state *c, unsigned int r, uint32_t *out) {
    if (!c || !out || r>=OBFH_UPREG_COUNT) return OBFH_UPREG_INVALID_ARGS;
    *out = c->controls[r] ^ obfh_upreg_control_mask(c,r);
    return OBFH_UPREG_OK;
}
static inline enum obfh_upreg_status obfh_upreg_set_flags(
        obfh_upreg_state *c, uint32_t value) {
    uint32_t variant;
    if (!c) return OBFH_UPREG_INVALID_ARGS;
    variant=obfh_upreg_draw(c->seed)%3u;
    c->flags = variant==0u ? (value ^ c->flag_key) :
               variant==1u ? (value + c->flag_key) :
                             (value - c->flag_key);
    /* Source OBFH_V_CONTEXT flags_ready is NOT set by obfh_v_set_flags. */
    return OBFH_UPREG_OK;
}
static inline enum obfh_upreg_status obfh_upreg_get_flags(
        const obfh_upreg_state *c, uint32_t *out) {
    uint32_t variant;
    if (!c || !out) return OBFH_UPREG_INVALID_ARGS;
    variant=obfh_upreg_draw(c->seed)%3u;
    *out=variant==0u ? (c->flags ^ c->flag_key) :
         variant==1u ? (c->flags - c->flag_key) :
                       (c->flags + c->flag_key);
    return OBFH_UPREG_OK;
}
static inline enum obfh_upreg_status obfh_upreg_initialize(
        obfh_upreg_state *c, uint32_t seed, uint32_t key) {
    unsigned int i;
    if (!c) return OBFH_UPREG_INVALID_ARGS;
    memset(c,0,sizeof *c);
    c->seed=seed;
    c->key=key;
    c->flag_key=obfh_upreg_mix(seed,key^UINT32_C(0x85ebca6b));
    (void)obfh_upreg_set_flags(c,0u);
    for (i=0u;i<OBFH_UPREG_COUNT;++i)
        (void)obfh_upreg_control_write(c,i,0u);
    return OBFH_UPREG_OK;
}
/* Preserve every object byte, including representation padding and NaN bits.
 * Commit the slot only after all bytes have been encoded. */
static inline enum obfh_upreg_status obfh_upreg_write(
        obfh_upreg_state *c, unsigned int r, const unsigned char *raw,
        size_t nbytes) {
    obfh_upreg_slot slot;
    uint32_t state;
    size_t k;
    if (!c || !raw || r>=OBFH_UPREG_COUNT ||
        nbytes!=sizeof(long double)) return OBFH_UPREG_INVALID_ARGS;
    state=obfh_upreg_mix(c->seed,
         c->key ^ ((uint32_t)r*UINT32_C(3266489917)) ^ c->steps);
    slot.mask=state;
    for (k=0u;k<nbytes;++k) {
        unsigned int i=(c->seed & (1u<<27u)) ? (unsigned int)(nbytes-1u-k)
                                            : (unsigned int)k;
        unsigned char m,src=raw[i];
        state=obfh_upreg_mix(c->seed,state+(uint32_t)i+1u);
        m=(unsigned char)state;
        switch ((c->seed>>8u)%3u) {
        case 0: slot.bytes[i]=(unsigned char)(src^m);break;
        case 1: slot.bytes[i]=(unsigned char)((unsigned int)src+m);break;
        default:slot.bytes[i]=(unsigned char)((unsigned int)src-m);break;
        }
    }
    c->registers[r]=slot;
    c->initialized|=1u<<r;
    return OBFH_UPREG_OK;
}
static inline enum obfh_upreg_status obfh_upreg_read(
        const obfh_upreg_state *c,unsigned int r,unsigned char *out,
        size_t nbytes) {
    unsigned char decoded[sizeof(long double)];
    uint32_t state;
    size_t k,i;
    if (!c || !out || r>=OBFH_UPREG_COUNT ||
        nbytes!=sizeof(long double)) return OBFH_UPREG_INVALID_ARGS;
    if (!(c->initialized&(1u<<r))) return OBFH_UPREG_UNINITIALIZED;
    state=c->registers[r].mask;
    for (k=0u;k<nbytes;++k) {
        unsigned char m,v;
        i=(c->seed & (1u<<27u)) ? nbytes-1u-k : k;
        state=obfh_upreg_mix(c->seed,state+(uint32_t)i+1u);
        m=(unsigned char)state;
        v=c->registers[r].bytes[i];
        switch ((c->seed>>8u)%3u) {
        case 0: decoded[i]=(unsigned char)(v^m);break;
        case 1: decoded[i]=(unsigned char)((unsigned int)v-m);break;
        default:decoded[i]=(unsigned char)((unsigned int)v+m);break;
        }
    }
    for (i=0u;i<nbytes;++i) out[i]=decoded[i];
    return OBFH_UPREG_OK;
}
static inline enum obfh_upreg_status obfh_upreg_copy(
        obfh_upreg_state *c,unsigned int dst,unsigned int src) {
    if (!c || dst>=OBFH_UPREG_COUNT || src>=OBFH_UPREG_COUNT)
        return OBFH_UPREG_INVALID_ARGS;
    if (!(c->initialized&(1u<<src))) return OBFH_UPREG_UNINITIALIZED;
    c->registers[dst]=c->registers[src];
    c->initialized|=1u<<dst;
    return OBFH_UPREG_OK;
}
static inline enum obfh_upreg_status obfh_upreg_swap(
        obfh_upreg_state *c,unsigned int a,unsigned int b) {
    obfh_upreg_slot temp;
    if (!c || a>=OBFH_UPREG_COUNT || b>=OBFH_UPREG_COUNT)
        return OBFH_UPREG_INVALID_ARGS;
    if (!(c->initialized&(1u<<a)) || !(c->initialized&(1u<<b)))
        return OBFH_UPREG_UNINITIALIZED;
    temp=c->registers[a];
    c->registers[a]=c->registers[b];
    c->registers[b]=temp;
    return OBFH_UPREG_OK;
}
/* Upstream control operations use encoded words, not native pointers.
 * Direct XOR of an encoded control is equivalent to XOR of decoded value.
 */
static inline enum obfh_upreg_status obfh_upreg_cxor(
        obfh_upreg_state *c,unsigned int r,uint32_t imm18) {
    if (!c || r>=OBFH_UPREG_COUNT || imm18>UINT32_C(0x3ffff))
        return OBFH_UPREG_INVALID_ARGS;
    c->controls[r]^=imm18;
    return OBFH_UPREG_OK;
}
static inline enum obfh_upreg_status obfh_upreg_crol(
        obfh_upreg_state *c,unsigned int r,uint32_t imm18) {
    uint32_t v;
    unsigned int n;
    if (!c || r>=OBFH_UPREG_COUNT || imm18>UINT32_C(0x3ffff))
        return OBFH_UPREG_INVALID_ARGS;
    (void)obfh_upreg_control_read(c,r,&v);
    n=(imm18%31u)+1u;
    v=(v<<n)|(v>>(32u-n));
    return obfh_upreg_control_write(c,r,v);
}
static inline enum obfh_upreg_status obfh_upreg_cadd(
        obfh_upreg_state *c,unsigned int r,uint32_t imm18) {
    uint32_t v,flags,delta;
    if (!c || r>=OBFH_UPREG_COUNT || imm18>UINT32_C(0x3ffff))
        return OBFH_UPREG_INVALID_ARGS;
    (void)obfh_upreg_control_read(c,r,&v);
    delta=(imm18&UINT32_C(0x20000)) ? (imm18|UINT32_C(0xfffc0000))
                                   :imm18;
    v+=delta;
    (void)obfh_upreg_control_write(c,r,v);
    (void)obfh_upreg_get_flags(c,&flags);
    (void)obfh_upreg_set_flags(c,(flags & ~OBFH_UPREG_CONTROL_FLAG) |
                                 (v ? OBFH_UPREG_CONTROL_FLAG:0u));
    c->flags_ready=1u;
    return OBFH_UPREG_OK;
}
#endif
