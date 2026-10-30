/* SPDX-License-Identifier: MIT
 * Verifies genuine encoded-word decode/dispatch and transactional reject.
 * Tests explicit bitwise API parity, not the upstream full VM bytecode format.
 */
#include <stdint.h>
#include "obfh_arm64_vm32_program.h"

typedef char vm32_public_types[
    __builtin_types_compatible_p(__typeof__(VM_AND(1u,2u)), unsigned int) &&
    __builtin_types_compatible_p(__typeof__(VM_OR(1u,2u)), unsigned int) &&
    __builtin_types_compatible_p(__typeof__(VM_XOR(1u,2u)), unsigned int) &&
    __builtin_types_compatible_p(__typeof__(VM_NOT(1u)), unsigned int) &&
    __builtin_types_compatible_p(__typeof__(VM_SHL(1u,2u)), unsigned int) &&
    __builtin_types_compatible_p(__typeof__(VM_SHR(1u,2u)), unsigned int)
    ? 1 : -1];

static int malformed_programs(void) {
    enum { key = OBFH_VM32_KEY(7u) };
    const uint32_t correct[4] = {
        OBFH_VM32_ENCODE(OBFH_VM32_PACK(OBFH_VM32_LOAD_A,0u,0u,0u),key,0u),
        OBFH_VM32_ENCODE(OBFH_VM32_PACK(OBFH_VM32_LOAD_B,1u,0u,0u),key,1u),
        OBFH_VM32_ENCODE(OBFH_VM32_PACK(OBFH_VM32_AND,2u,0u,1u),key,2u),
        OBFH_VM32_ENCODE(OBFH_VM32_PACK(OBFH_VM32_RETURN,0u,2u,0u),key,3u)
    };
    uint32_t modified[4], output, steps;
    unsigned int i;
#define REJECT(expectedStatus, length, secretKey, expectedOp) do { \
    output = 0xcafebabeu; steps = 0xdecafbadU; \
    if (obfh_arm64_vm32_execute(modified,(length),(secretKey),(expectedOp), \
            0xf0f0c0c0u,0xff00ff00u,&output,&steps) != (expectedStatus) || \
        output != 0xcafebabeu || steps != 0xdecafbadu) return 20; \
} while (0)

    for (i = 0; i < 4; ++i) modified[i] = correct[i];
    output = 0u; steps = 0u;
    if (obfh_arm64_vm32_execute(correct,4u,key,OBFH_VM32_AND,
            0xf0f0c0c0u,0xff00ff00u,&output,&steps) != OBFH_VM32_OK ||
        output != (0xf0f0c0c0u & 0xff00ff00u) || steps != 4u) return 10;
    REJECT(OBFH_VM32_INVALID_ARGS,3u,key,OBFH_VM32_AND);
    REJECT(OBFH_VM32_INVALID_ARGS,5u,key,OBFH_VM32_AND);
    REJECT(OBFH_VM32_INVALID_ARGS,4u,key,255u);
    REJECT(OBFH_VM32_WRONG_OPERATION,4u,key,OBFH_VM32_XOR);
    /* An unexpected 32-bit opcode, register, reserved bit or terminal must
     * never be treated as valid, nor update partial caller-owned state.
     */
    modified[2] ^= 0x80u;
    REJECT(OBFH_VM32_WRONG_OPERATION,4u,key,OBFH_VM32_AND);
    modified[2] = correct[2] ^ 0x4000u;
    REJECT(OBFH_VM32_INVALID_WORD,4u,key,OBFH_VM32_AND);
    modified[2] = correct[2] ^ 0x100u;
    REJECT(OBFH_VM32_WRONG_OPERATION,4u,key,OBFH_VM32_AND);
    modified[2] = correct[2];
    modified[0] ^= 0x1u;
    REJECT(OBFH_VM32_INVALID_WORD,4u,key,OBFH_VM32_AND);
    modified[0] = correct[0];
    modified[3] ^= 0x1u;
    REJECT(OBFH_VM32_INVALID_WORD,4u,key,OBFH_VM32_AND);
    modified[3] = correct[3];
    /* Key mismatch can yield either invalid opcode shape or reserved bits. */
    output = 0xcafebabeu; steps = 0xdecafbadu;
    if (obfh_arm64_vm32_execute(modified,4u,key ^ 0x55555555u,
             OBFH_VM32_AND,7u,3u,&output,&steps) == OBFH_VM32_OK ||
        output != 0xcafebabeu || steps != 0xdecafbadu) return 21;
    if (obfh_arm64_vm32_execute(NULL,4u,key,OBFH_VM32_AND,
             7u,3u,&output,&steps) != OBFH_VM32_INVALID_ARGS) return 22;
    if (obfh_arm64_vm32_execute(correct,4u,key,OBFH_VM32_AND,
             7u,3u,NULL,&steps) != OBFH_VM32_INVALID_ARGS) return 23;
    if (obfh_arm64_vm32_execute(correct,4u,key,OBFH_VM32_AND,
             7u,3u,&output,NULL) != OBFH_VM32_INVALID_ARGS) return 24;
#undef REJECT
    return 0;
}

static int side_effects(void) {
    unsigned int a, b;
#define ONCE(OP) do { a=0u;b=0u;(void)OP(++a,++b); if(a!=1u||b!=1u)return 31; } while(0)
    ONCE(VM_AND);
    ONCE(VM_OR);
    ONCE(VM_XOR);
    ONCE(VM_SHL);
    ONCE(VM_SHR);
#undef ONCE
    a=0u;
    if (VM_NOT(++a) != ~1u || a != 1u) return 32;
    return 0;
}

int main(void) {
    static const uint32_t samples[] = {
        0u, 1u, 31u, 32u, 0x55555555u, 0x80000000u, 0xffffffffu
    };
    uint32_t state = 0x91745123u;
    unsigned int i, j;
    int failures = malformed_programs();
    if (failures) return failures;
    failures = side_effects();
    if (failures) return failures;

    for (i = 0; i < 7u; ++i) {
        for (j = 0; j < 7u; ++j) {
            uint32_t a=samples[i], b=samples[j];
            if (VM_AND(a,b)!=(a&b)) return 41;
            if (VM_OR(a,b)!=(a|b)) return 42;
            if (VM_XOR(a,b)!=(a^b)) return 43;
            if (VM_NOT(a)!=(~a)) return 44;
            if (VM_SHL(a,b)!=(a<<(b&31u))) return 45;
            if (VM_SHR(a,b)!=(a>>(b&31u))) return 46;
        }
    }
    for (i=0u;i<4000u;++i) {
        uint32_t a,b;
        state=state*1664525u+1013904223u; a=state;
        state=state*1664525u+1013904223u; b=state;
        if (VM_AND(a,b)!=(a&b) || VM_OR(a,b)!=(a|b)) return 51;
        if (VM_XOR(a,b)!=(a^b) || VM_NOT(a)!=(~a)) return 52;
        if (VM_SHL(a,b)!=(a<<(b&31u)) || VM_SHR(a,b)!=(a>>(b&31u))) return 53;
    }
    if (VM_XOR(VM_NOT(0u),VM_SHL(1u,31u)) != 0x7fffffffu) return 54;
    return 0;
}
