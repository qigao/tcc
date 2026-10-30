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
#if UINTPTR_MAX != UINT64_MAX
#error "This experiment requires AArch64 with 64-bit uintptr_t."
#endif

/* Pure C seed mixing: only unsigned modular arithmetic. */
static inline uint32_t obfh_arm64_mix32(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

/* These A64 instruction forms preserve SP, LR, callee-saved registers and
 * all memory. The compiler allocates registers through output constraints. */
static inline uint32_t obfh_arm64_vm_add(uint32_t left, uint32_t right) {
    const uint32_t key = obfh_arm64_mix32((uint32_t)OBFH_SEED ^ 0x41444431u);
#if (OBFH_SEED & 1u)
    asm volatile("add %w0, %w0, %w1\n\tsub %w0, %w0, %w1"
                     : "+&r"(left) : "r"(key) : "cc");
#else
    asm volatile("eor %w0, %w0, %w1\n\teor %w0, %w0, %w1"
                     : "+&r"(left) : "r"(key) : "cc");
#endif
    asm volatile("add %w0, %w0, %w1" : "+&r"(left) : "r"(right) : "cc");
    return left;
}

static inline uint32_t obfh_arm64_vm_xor(uint32_t left, uint32_t right) {
    asm volatile("eor %w0, %w0, %w1" : "+&r"(left) : "r"(right) : "cc");
    return left;
}

/* A64 computes the actual condition; compiler C owns branch and return.
 * A seed-selected bijection preserves equality without changing operands. */
static inline int obfh_arm64_branch_equal(uint32_t left, uint32_t right) {
    const uint32_t key = obfh_arm64_mix32((uint32_t)OBFH_SEED ^ 0x42524e43u);
    const uint32_t masked_left = left ^ key;
    const uint32_t masked_right = right ^ key;
    uint32_t difference;
    asm volatile("eor %w0, %w1, %w2"
                 : "=&r"(difference)
                 : "r"(masked_left), "r"(masked_right)
                 : "cc");
    return difference == 0u;
}

/* Encode a borrowed object pointer to an integer token, then decode before
 * any dereference. Never use the encoded value as a pointer or transfer
 * ownership through the token. The roundtrip preserves every pointer bit. */
static inline uintptr_t obfh_arm64_pointer_key(void) {
    const uintptr_t high =
        (uintptr_t)obfh_arm64_mix32((uint32_t)OBFH_SEED ^ 0x504f494eu);
    const uintptr_t low =
        (uintptr_t)obfh_arm64_mix32((uint32_t)OBFH_SEED ^ 0x544f4b4eu);
    return (high << 32) | low;
}

static inline uintptr_t obfh_arm64_pointer_encode(const void *pointer) {
    uintptr_t value = (uintptr_t)pointer;
    const uintptr_t key = obfh_arm64_pointer_key();
#if (OBFH_SEED & 1u)
    asm volatile("eor %0, %0, %1\n\tror %0, %0, #17"
                 : "+&r"(value) : "r"(key));
#else
    asm volatile("add %0, %0, %1\n\teor %0, %0, %1"
                 : "+&r"(value) : "r"(key));
#endif
    return value;
}

static inline void *obfh_arm64_pointer_decode(uintptr_t token) {
    const uintptr_t key = obfh_arm64_pointer_key();
#if (OBFH_SEED & 1u)
    asm volatile("ror %0, %0, #47\n\teor %0, %0, %1"
                 : "+&r"(token) : "r"(key));
#else
    asm volatile("eor %0, %0, %1\n\tsub %0, %0, %1"
                 : "+&r"(token) : "r"(key));
#endif
    return (void *)token;
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
