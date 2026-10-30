/* SPDX-License-Identifier: MIT
 * Exact physical field orders of upstream obfus.h OBFH_V_CONTEXT for ARM64.
 * Source SHA 4c06fbc06f907afb3989e2851bed7583cfd0b6b0 (region 11).
 * The upstream uses unsigned int; ARM64 requires sizeof(uint32_t)==sizeof
 * unsigned int. These are C structs, NOT packed nor user-chosen field order.
 * Existing semantic VM state remains the only runtime; this layout adapter
 * is a separate ABI qualification, not an alternative VM execution engine.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_CONTEXT_H
#define QIGAO_OBFH_ARM64_UPSTREAM_CONTEXT_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "obfh_arm64_upstream_registers.h"

#if UINT_MAX != UINT32_MAX
#error "Original obfus.h OBFH_V_CONTEXT requires uint32_t unsigned int"
#endif
_Static_assert(sizeof(obfh_upreg_slot) ==
                ((sizeof(long double)+sizeof(uint32_t)+_Alignof(uint32_t)-1u) /
                 _Alignof(uint32_t))*_Alignof(uint32_t),
               "Original OBFH_V_REGISTER physical size mismatch");

typedef struct {
    obfh_upreg_slot registers[OBFH_UPREG_COUNT];
    uint32_t controls[OBFH_UPREG_COUNT];
    uint32_t flags;
    uint32_t flag_key;
    uint32_t pc;
    uint32_t steps;
    uint32_t initialized;
    uint32_t key;
    uint32_t status;
    uint32_t flags_ready;
    long double result;
} obfh_upcontext_layout_0;

typedef struct {
    long double result;
    uint32_t key;
    uint32_t initialized;
    uint32_t steps;
    uint32_t pc;
    uint32_t flags_ready;
    uint32_t status;
    uint32_t flag_key;
    uint32_t flags;
    uint32_t controls[OBFH_UPREG_COUNT];
    obfh_upreg_slot registers[OBFH_UPREG_COUNT];
} obfh_upcontext_layout_1;

typedef struct {
    uint32_t controls[OBFH_UPREG_COUNT];
    uint32_t pc;
    uint32_t key;
    uint32_t status;
    uint32_t flags;
    uint32_t steps;
    uint32_t flag_key;
    uint32_t flags_ready;
    uint32_t initialized;
    long double result;
    obfh_upreg_slot registers[OBFH_UPREG_COUNT];
} obfh_upcontext_layout_2;

typedef struct {
    uint32_t status;
    uint32_t flags_ready;
    uint32_t key;
    uint32_t initialized;
    uint32_t flag_key;
    uint32_t steps;
    uint32_t flags;
    uint32_t pc;
    obfh_upreg_slot registers[OBFH_UPREG_COUNT];
    long double result;
    uint32_t controls[OBFH_UPREG_COUNT];
} obfh_upcontext_layout_3;

#if ((OBFH_SEED >> 4u) % 4u) == 0u
typedef obfh_upcontext_layout_0 obfh_upcontext_selected;
#elif ((OBFH_SEED >> 4u) % 4u) == 1u
typedef obfh_upcontext_layout_1 obfh_upcontext_selected;
#elif ((OBFH_SEED >> 4u) % 4u) == 2u
typedef obfh_upcontext_layout_2 obfh_upcontext_selected;
#else
typedef obfh_upcontext_layout_3 obfh_upcontext_selected;
#endif


static inline int obfh_upcontext_export_0(
        obfh_upcontext_layout_0 *dest,
        const obfh_upreg_state *state,
        uint32_t pc, uint32_t status, const long double *result) {
    if (!dest || !state || !result) return 0;
    memset(dest,0,sizeof *dest);
    memcpy(dest->registers,state->registers,sizeof state->registers);
    memcpy(dest->controls,state->controls,sizeof state->controls);
    dest->flags=state->flags;
    dest->flag_key=state->flag_key;
    dest->pc=pc;
    dest->steps=state->steps;
    dest->initialized=state->initialized;
    dest->key=state->key;
    dest->status=status;
    dest->flags_ready=state->flags_ready;
    memcpy(&dest->result,result,sizeof *result);
    return 1;
}
static inline int obfh_upcontext_import_0(
        obfh_upreg_state *state,
        uint32_t *pc, uint32_t *status, long double *result,
        const obfh_upcontext_layout_0 *src, uint32_t seed) {
    obfh_upreg_state recovered;
    if (!state || !pc || !status || !result || !src) return 0;
    memset(&recovered,0,sizeof recovered);
    memcpy(recovered.registers,src->registers,sizeof recovered.registers);
    memcpy(recovered.controls,src->controls,sizeof recovered.controls);
    recovered.seed=seed;
    recovered.flags=src->flags;
    recovered.flag_key=src->flag_key;
    recovered.steps=src->steps;
    recovered.initialized=src->initialized;
    recovered.key=src->key;
    recovered.flags_ready=src->flags_ready;
    *state=recovered;
    *pc=src->pc;
    *status=src->status;
    memcpy(result,&src->result,sizeof *result);
    return 1;
}


static inline int obfh_upcontext_export_1(
        obfh_upcontext_layout_1 *dest,
        const obfh_upreg_state *state,
        uint32_t pc, uint32_t status, const long double *result) {
    if (!dest || !state || !result) return 0;
    memset(dest,0,sizeof *dest);
    memcpy(dest->registers,state->registers,sizeof state->registers);
    memcpy(dest->controls,state->controls,sizeof state->controls);
    dest->flags=state->flags;
    dest->flag_key=state->flag_key;
    dest->pc=pc;
    dest->steps=state->steps;
    dest->initialized=state->initialized;
    dest->key=state->key;
    dest->status=status;
    dest->flags_ready=state->flags_ready;
    memcpy(&dest->result,result,sizeof *result);
    return 1;
}
static inline int obfh_upcontext_import_1(
        obfh_upreg_state *state,
        uint32_t *pc, uint32_t *status, long double *result,
        const obfh_upcontext_layout_1 *src, uint32_t seed) {
    obfh_upreg_state recovered;
    if (!state || !pc || !status || !result || !src) return 0;
    memset(&recovered,0,sizeof recovered);
    memcpy(recovered.registers,src->registers,sizeof recovered.registers);
    memcpy(recovered.controls,src->controls,sizeof recovered.controls);
    recovered.seed=seed;
    recovered.flags=src->flags;
    recovered.flag_key=src->flag_key;
    recovered.steps=src->steps;
    recovered.initialized=src->initialized;
    recovered.key=src->key;
    recovered.flags_ready=src->flags_ready;
    *state=recovered;
    *pc=src->pc;
    *status=src->status;
    memcpy(result,&src->result,sizeof *result);
    return 1;
}


static inline int obfh_upcontext_export_2(
        obfh_upcontext_layout_2 *dest,
        const obfh_upreg_state *state,
        uint32_t pc, uint32_t status, const long double *result) {
    if (!dest || !state || !result) return 0;
    memset(dest,0,sizeof *dest);
    memcpy(dest->registers,state->registers,sizeof state->registers);
    memcpy(dest->controls,state->controls,sizeof state->controls);
    dest->flags=state->flags;
    dest->flag_key=state->flag_key;
    dest->pc=pc;
    dest->steps=state->steps;
    dest->initialized=state->initialized;
    dest->key=state->key;
    dest->status=status;
    dest->flags_ready=state->flags_ready;
    memcpy(&dest->result,result,sizeof *result);
    return 1;
}
static inline int obfh_upcontext_import_2(
        obfh_upreg_state *state,
        uint32_t *pc, uint32_t *status, long double *result,
        const obfh_upcontext_layout_2 *src, uint32_t seed) {
    obfh_upreg_state recovered;
    if (!state || !pc || !status || !result || !src) return 0;
    memset(&recovered,0,sizeof recovered);
    memcpy(recovered.registers,src->registers,sizeof recovered.registers);
    memcpy(recovered.controls,src->controls,sizeof recovered.controls);
    recovered.seed=seed;
    recovered.flags=src->flags;
    recovered.flag_key=src->flag_key;
    recovered.steps=src->steps;
    recovered.initialized=src->initialized;
    recovered.key=src->key;
    recovered.flags_ready=src->flags_ready;
    *state=recovered;
    *pc=src->pc;
    *status=src->status;
    memcpy(result,&src->result,sizeof *result);
    return 1;
}


static inline int obfh_upcontext_export_3(
        obfh_upcontext_layout_3 *dest,
        const obfh_upreg_state *state,
        uint32_t pc, uint32_t status, const long double *result) {
    if (!dest || !state || !result) return 0;
    memset(dest,0,sizeof *dest);
    memcpy(dest->registers,state->registers,sizeof state->registers);
    memcpy(dest->controls,state->controls,sizeof state->controls);
    dest->flags=state->flags;
    dest->flag_key=state->flag_key;
    dest->pc=pc;
    dest->steps=state->steps;
    dest->initialized=state->initialized;
    dest->key=state->key;
    dest->status=status;
    dest->flags_ready=state->flags_ready;
    memcpy(&dest->result,result,sizeof *result);
    return 1;
}
static inline int obfh_upcontext_import_3(
        obfh_upreg_state *state,
        uint32_t *pc, uint32_t *status, long double *result,
        const obfh_upcontext_layout_3 *src, uint32_t seed) {
    obfh_upreg_state recovered;
    if (!state || !pc || !status || !result || !src) return 0;
    memset(&recovered,0,sizeof recovered);
    memcpy(recovered.registers,src->registers,sizeof recovered.registers);
    memcpy(recovered.controls,src->controls,sizeof recovered.controls);
    recovered.seed=seed;
    recovered.flags=src->flags;
    recovered.flag_key=src->flag_key;
    recovered.steps=src->steps;
    recovered.initialized=src->initialized;
    recovered.key=src->key;
    recovered.flags_ready=src->flags_ready;
    *state=recovered;
    *pc=src->pc;
    *status=src->status;
    memcpy(result,&src->result,sizeof *result);
    return 1;
}


/* The sole integrated VM still owns all instruction semantics. This union
 * owns the authoritative encoded state BETWEEN instructions; the existing
 * obfh_upreg_state is used only as a stack-local decoded staging view for
 * one instruction, and is immediately committed back to original-layout
 * physical storage. No second VM/interpreter or pointer alias UB.
 *
 * For an emitted call with its own seed, the source's (seed >> 4) % 4 picks
 * exactly one active union member. A TU can test all four runtime seeds;
 * the selected ABI typedef above remains bound to its build OBFH_SEED.
 */
#define OBFH_UPCONTEXT_LIVE_COOKIE UINT32_C(0x75504358)
typedef union {
    obfh_upcontext_layout_0 mode0;
    obfh_upcontext_layout_1 mode1;
    obfh_upcontext_layout_2 mode2;
    obfh_upcontext_layout_3 mode3;
} obfh_upcontext_live_bytes;

typedef struct {
    obfh_upcontext_live_bytes physical;
    uint32_t seed;
    uint32_t mode;
    uint32_t cookie;
} obfh_upcontext_live;

/* Caller must have performed a seed/key-aware obfh_upreg_initialize before
 * the first store. Every write to an original physical context is complete:
 * all four encoded slots, controls, flags, PC, steps, status and result.
 */
static inline int obfh_upcontext_live_store(
        obfh_upcontext_live *owner, const obfh_upreg_state *state,
        uint32_t pc, uint32_t status, const long double *result) {
    if (!owner || !state || !result ||
        owner->cookie!=OBFH_UPCONTEXT_LIVE_COOKIE ||
        owner->seed!=state->seed ||
        owner->mode!=((owner->seed>>4u)%4u))
        return 0;
    switch (owner->mode) {
    case 0u: return obfh_upcontext_export_0(&owner->physical.mode0,
                                               state,pc,status,result);
    case 1u: return obfh_upcontext_export_1(&owner->physical.mode1,
                                               state,pc,status,result);
    case 2u: return obfh_upcontext_export_2(&owner->physical.mode2,
                                               state,pc,status,result);
    case 3u: return obfh_upcontext_export_3(&owner->physical.mode3,
                                               state,pc,status,result);
    default: return 0;
    }
}
static inline int obfh_upcontext_live_load(
        const obfh_upcontext_live *owner, obfh_upreg_state *state,
        uint32_t *pc, uint32_t *status, long double *result) {
    if (!owner || !state || !pc || !status || !result ||
        owner->cookie!=OBFH_UPCONTEXT_LIVE_COOKIE ||
        owner->mode!=((owner->seed>>4u)%4u))
        return 0;
    switch (owner->mode) {
    case 0u: return obfh_upcontext_import_0(state,pc,status,result,
                                     &owner->physical.mode0,owner->seed);
    case 1u: return obfh_upcontext_import_1(state,pc,status,result,
                                     &owner->physical.mode1,owner->seed);
    case 2u: return obfh_upcontext_import_2(state,pc,status,result,
                                     &owner->physical.mode2,owner->seed);
    case 3u: return obfh_upcontext_import_3(state,pc,status,result,
                                     &owner->physical.mode3,owner->seed);
    default: return 0;
    }
}
static inline int obfh_upcontext_live_initialize(
        obfh_upcontext_live *owner, uint32_t seed, uint32_t key) {
    obfh_upreg_state initial;
    long double initial_result=0.0L;
    if (!owner ||
        obfh_upreg_initialize(&initial,seed,key)!=OBFH_UPREG_OK)
        return 0;
    memset(owner,0,sizeof *owner);
    owner->seed=seed;
    owner->mode=(seed>>4u)%4u;
    owner->cookie=OBFH_UPCONTEXT_LIVE_COOKIE;
    return obfh_upcontext_live_store(owner,&initial,0u,0u,&initial_result);
}

#endif
