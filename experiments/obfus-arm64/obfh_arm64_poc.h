/* SPDX-License-Identifier: MIT
 * ARM64 instruction/ABI feasibility experiment, NOT a port of obfus.h.
 */
#ifndef QIGAO_OBFH_ARM64_POC_H
#define QIGAO_OBFH_ARM64_POC_H

#include <stddef.h>
#include <stdint.h>

#if !defined(__aarch64__)
#error "This obfus.h feasibility POC requires an AArch64 target."
#endif
#if !defined(OBFH_SEED)
#error "Define OBFH_SEED to an unsigned 32-bit build constant."
#endif
#if defined(__TINYC__) && __TINYC__ < 928
#error "TinyCC 0.9.28rc or newer is required for ARM64 extended inline asm."
#endif

/* All transformations are in uint32_t. Neither SP/LR nor callee-saved
 * registers are referenced. Extended asm allocates scratch registers. */
static inline uint32_t obfh_arm64_mix32(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

static inline uint32_t obfh_arm64_vm_add(uint32_t left, uint32_t right) {
    const uint32_t key = obfh_arm64_mix32((uint32_t)OBFH_SEED ^ 0x41444431u);
#if (OBFH_SEED & 1u)
    asm volatile("add %w0, %w0, %w1\n\tsub %w0, %w0, %w1"
                     : "+r"(left) : "r"(key) : "cc");
#else
    asm volatile("eor %w0, %w0, %w1\n\teor %w0, %w0, %w1"
                     : "+r"(left) : "r"(key) : "cc");
#endif
    asm volatile("add %w0, %w0, %w1" : "+r"(left) : "r"(right) : "cc");
    return left;
}

static inline uint32_t obfh_arm64_vm_xor(uint32_t left, uint32_t right) {
    asm volatile("eor %w0, %w0, %w1" : "+r"(left) : "r"(right) : "cc");
    return left;
}

static inline int obfh_arm64_branch_equal(uint32_t left, uint32_t right) {
    const uint32_t key = obfh_arm64_mix32((uint32_t)OBFH_SEED ^ 0x42524e43u);
    asm volatile("eor %w0, %w0, %w1\n\teor %w0, %w0, %w1"
                     : "+r"(left) : "r"(key) : "cc");
    return left == right;
}

/* Generated fixture bytes contain no plaintext; seed is a build input.
 * The caller owns output, including its decoded NUL byte and lifetime. */
static inline void obfh_arm64_decode_bytes(unsigned char *out,
                                            const unsigned char *encoded,
                                            size_t count) {
    uint32_t state = (uint32_t)OBFH_SEED ^ 0x41524d36u;
    size_t i;
    for (i = 0; i < count; ++i) {
        state = obfh_arm64_mix32(state + (uint32_t)(i + 1u) * 0x9e3779b9u);
        out[i] = (unsigned char)(encoded[i] ^ (unsigned char)state);
    }
}

#endif
