/* SPDX-License-Identifier: MIT
 * Single shared encoding contract for isolated AArch64 VM32 POC executors.
 * Not the upstream obfus.h opcode layout or a production bytecode ABI.
 */
#ifndef QIGAO_OBFH_ARM64_VM32_CODEC_H
#define QIGAO_OBFH_ARM64_VM32_CODEC_H
#include "obfh_arm64_poc.h"

enum {
    OBFH_VM32_LOAD_A = 1u,
    OBFH_VM32_LOAD_B = 2u,
    OBFH_VM32_AND = 3u,
    OBFH_VM32_OR = 4u,
    OBFH_VM32_XOR = 5u,
    OBFH_VM32_NOT = 6u,
    OBFH_VM32_SHL = 7u,
    OBFH_VM32_SHR = 8u,
    OBFH_VM32_RETURN = 9u
};

/* Shared POC encoding: op[7:0], d[9:8], a[11:10], b[13:12].
 * The control-flow extension exclusively owns target[23:14].
 * This is NOT upstream's VM field permutation or nonce format.
 */
#define OBFH_VM32_PACK(op,d,a,b) \
    ((uint32_t)(op) | ((uint32_t)(d) << 8u) | \
     ((uint32_t)(a) << 10u) | ((uint32_t)(b) << 12u))
#define OBFH_VM32_KEY(site) \
    ((uint32_t)OBFH_SEED ^ UINT32_C(0x7a335f19) ^ \
     (((uint32_t)(site) + 1u) * UINT32_C(0x9e3779b9)))
#define OBFH_VM32_MASK(key,pc) \
    ((uint32_t)(key) ^ (((uint32_t)(pc) + 1u) * UINT32_C(0x85ebca6b)))
#define OBFH_VM32_ENCODE(word,key,pc) \
    ((uint32_t)(word) ^ OBFH_VM32_MASK((key),(pc)))
#endif
