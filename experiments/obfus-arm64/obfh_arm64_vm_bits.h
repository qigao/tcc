/* SPDX-License-Identifier: MIT
 * Source-API subset experiment: NOT the original VM bytecode dispatcher.
 * Include explicitly in isolated AArch64 POC builds; do not substitute for
 * DosX-dev/obfus.h or silently imply unsupported VM_* APIs are present.
 */
#ifndef QIGAO_OBFH_ARM64_VM_BITS_H
#define QIGAO_OBFH_ARM64_VM_BITS_H

#include "obfh_arm64_poc.h"

#if defined(VM_AND) || defined(VM_OR) || defined(VM_XOR) || \
    defined(VM_NOT) || defined(VM_SHL) || defined(VM_SHR)
#error "Existing VM_* macros conflict with the experimental ARM64 subset."
#endif

/* Same public bitwise result ABI as obfus.h region 14: unsigned int.
 * Inputs are converted once by wrapper macros. Only genuine A64 assembler
 * performs the operation; there is no C implementation fallback.
 * These are *direct* ISA operations, not VM instruction virtualization.
 */
static inline unsigned int obfh_arm64_bit_and(unsigned int left, unsigned int right) {
    unsigned int out;
    const unsigned int key = obfh_arm64_mix32((unsigned int)OBFH_SEED ^ 0x41363442u);
#if (OBFH_SEED & 1u)
    asm volatile("eor %w0, %w1, %w3\n\teor %w0, %w0, %w3\n\tand %w0, %w0, %w2"
                 : "=&r"(out) : "r"(left), "r"(right), "r"(key));
#else
    asm volatile("add %w0, %w1, %w3\n\tsub %w0, %w0, %w3\n\tand %w0, %w0, %w2"
                 : "=&r"(out) : "r"(left), "r"(right), "r"(key));
#endif
    return out;
}
static inline unsigned int obfh_arm64_bit_or(unsigned int left, unsigned int right) {
    unsigned int out;
    asm volatile("orr %w0, %w1, %w2" : "=&r"(out) : "r"(left), "r"(right));
    return out;
}
static inline unsigned int obfh_arm64_bit_xor(unsigned int left, unsigned int right) {
    unsigned int out;
    asm volatile("eor %w0, %w1, %w2" : "=&r"(out) : "r"(left), "r"(right));
    return out;
}
static inline unsigned int obfh_arm64_bit_not(unsigned int value) {
    unsigned int out;
    const unsigned int ones = ~0u;
    asm volatile("eor %w0, %w1, %w2" : "=&r"(out) : "r"(value), "r"(ones));
    return out;
}
static inline unsigned int obfh_arm64_bit_shl(unsigned int value, unsigned int count) {
    unsigned int out;
    const unsigned int normalized_count = count & 31u;
    asm volatile("lsl %w0, %w1, %w2"
                 : "=&r"(out) : "r"(value), "r"(normalized_count));
    return out;
}
static inline unsigned int obfh_arm64_bit_shr(unsigned int value, unsigned int count) {
    unsigned int out;
    const unsigned int normalized_count = count & 31u;
    asm volatile("lsr %w0, %w1, %w2"
                 : "=&r"(out) : "r"(value), "r"(normalized_count));
    return out;
}

#define VM_AND(a, b) obfh_arm64_bit_and((unsigned int)(a), (unsigned int)(b))
#define VM_OR(a, b) obfh_arm64_bit_or((unsigned int)(a), (unsigned int)(b))
#define VM_XOR(a, b) obfh_arm64_bit_xor((unsigned int)(a), (unsigned int)(b))
#define VM_NOT(a) obfh_arm64_bit_not((unsigned int)(a))
#define VM_SHL(a, b) obfh_arm64_bit_shl((unsigned int)(a), (unsigned int)(b))
#define VM_SHR(a, b) obfh_arm64_bit_shr((unsigned int)(a), (unsigned int)(b))

#endif
