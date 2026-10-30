/* SPDX-License-Identifier: MIT
 * Compare the six explicit ARM64 VM_* bitwise source signatures with the
 * original obfus.h tests/vm.c bitwise contract. This is NOT the VM dispatcher.
 */
#include <stdint.h>
#include "obfh_arm64_vm_bits.h"

#if defined(VM_ADD) || defined(VM_IF) || defined(VM_DIV) || defined(VM_ADD_DBL)
#error "Unported original obfus.h VM operations must not be exported."
#endif

typedef char obfh_bits_public_typecheck[
    __builtin_types_compatible_p(__typeof__(VM_AND(0u, 0u)), unsigned int) &&
    __builtin_types_compatible_p(__typeof__(VM_OR(0u, 0u)), unsigned int) &&
    __builtin_types_compatible_p(__typeof__(VM_XOR(0u, 0u)), unsigned int) &&
    __builtin_types_compatible_p(__typeof__(VM_NOT(0u)), unsigned int) &&
    __builtin_types_compatible_p(__typeof__(VM_SHL(0u, 0u)), unsigned int) &&
    __builtin_types_compatible_p(__typeof__(VM_SHR(0u, 0u)), unsigned int) ? 1 : -1];

static int check_side_effects(void) {
    unsigned int a, b;
#define TEST_BINARY_ONE_EVAL(OP) do { \
    a = 0; b = 0; (void)OP(++a, ++b); \
    if (a != 1u || b != 1u) return 21; \
} while (0)
    TEST_BINARY_ONE_EVAL(VM_AND);
    TEST_BINARY_ONE_EVAL(VM_OR);
    TEST_BINARY_ONE_EVAL(VM_XOR);
    TEST_BINARY_ONE_EVAL(VM_SHL);
    TEST_BINARY_ONE_EVAL(VM_SHR);
#undef TEST_BINARY_ONE_EVAL
    a = 0;
    if (VM_NOT(++a) != ~1u || a != 1u) return 22;
    return 0;
}

int main(void) {
    static const unsigned int samples[] = {
        0u, 1u, 31u, 32u, 0x55555555u, 0x80000000u, 0xffffffffu
    };
    unsigned int seed = 0x91745123u;
    unsigned int i, j;
    int side = check_side_effects();
    if (side) return side;
    for (i = 0; i < sizeof samples / sizeof samples[0]; ++i) {
        for (j = 0; j < sizeof samples / sizeof samples[0]; ++j) {
            const unsigned int a = samples[i], b = samples[j];
            if (VM_AND(a, b) != (a & b)) return 31;
            if (VM_OR(a, b) != (a | b)) return 32;
            if (VM_XOR(a, b) != (a ^ b)) return 33;
            if (VM_NOT(a) != (~a)) return 34;
            if (VM_SHL(a, b) != (a << (b & 31u))) return 35;
            if (VM_SHR(a, b) != (a >> (b & 31u))) return 36;
        }
    }
    for (i = 0; i < 4000u; ++i) {
        unsigned int a, b;
        seed = seed * 1664525u + 1013904223u;
        a = seed;
        seed = seed * 1664525u + 1013904223u;
        b = seed;
        if (VM_AND(a, b) != (a & b) || VM_OR(a, b) != (a | b)) return 41;
        if (VM_XOR(a, b) != (a ^ b) || VM_NOT(a) != (~a)) return 42;
        if (VM_SHL(a, b) != (a << (b & 31u)) ||
            VM_SHR(a, b) != (a >> (b & 31u))) return 43;
    }
    if (VM_AND(-1, 0x80000000u) != 0x80000000u) return 44;
    if (VM_SHR(-1, 31) != 1u) return 45;
    if (VM_XOR(VM_NOT(0u), VM_SHL(1u, 31u)) != 0x7fffffffu) return 46;
    return 0;
}
