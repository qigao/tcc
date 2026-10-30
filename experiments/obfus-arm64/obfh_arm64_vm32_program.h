/* SPDX-License-Identifier: MIT
 * A bounded encoded-instruction AArch64 VM experiment, NOT upstream VM parity.
 * Four immutable words: LOAD_A, LOAD_B, one 32-bit bitwise ALU op, RETURN.
 * Unlike direct ISA adapters this decodes and DISPATCHES each encoded word.
 * Deliberately no jump, floating-point, arithmetic or automatic-CFLOW claims.
 */
#ifndef QIGAO_OBFH_ARM64_VM32_PROGRAM_H
#define QIGAO_OBFH_ARM64_VM32_PROGRAM_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "obfh_arm64_poc.h"

#if defined(VM_AND) || defined(VM_OR) || defined(VM_XOR) || \
    defined(VM_NOT) || defined(VM_SHL) || defined(VM_SHR)
#error "Cannot combine two VM_* implementations in one translation unit."
#endif

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
enum obfh_vm32_status {
    OBFH_VM32_OK = 0,
    OBFH_VM32_INVALID_ARGS = 1,
    OBFH_VM32_INVALID_WORD = 2,
    OBFH_VM32_WRONG_OPERATION = 3
};
#define OBFH_VM32_PROGRAM_WORDS 4u
/* Layout: opcode[7:0], destination[9:8], source-A[11:10],
 * source-B[13:12]. The top 18 bits MUST be zero after decoding.
 * All operations below are 32-bit unsigned, with shifts masked mod 32.
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

/* Never trust decoded input. The immutable four-word shape and complete
 * register schedule are validated BEFORE any operation updates output.
 * The native A64 asm always decodes, then the runtime switch dispatches.
 * Output and executed-step receipt are unchanged on any malformed program.
 * noinline prevents optimizing the encoded runtime program into a C constant.
 */
static __attribute__((noinline)) enum obfh_vm32_status
obfh_arm64_vm32_execute(const uint32_t *program, size_t length,
                        uint32_t key, unsigned int expected_operation,
                        uint32_t input_a, uint32_t input_b,
                        uint32_t *output, uint32_t *executed_steps) {
    uint32_t registers[3] = {0u, 0u, 0u};
    uint32_t decoded[OBFH_VM32_PROGRAM_WORDS];
    unsigned int pc;

    if (!program || !output || !executed_steps ||
        length != OBFH_VM32_PROGRAM_WORDS ||
        expected_operation < OBFH_VM32_AND ||
        expected_operation > OBFH_VM32_SHR)
        return OBFH_VM32_INVALID_ARGS;

    /* First pass rejects *all* malformed words, including trailing entries.
     * This is a transaction: caller-visible output is only written on success.
     */
    for (pc = 0u; pc < OBFH_VM32_PROGRAM_WORDS; ++pc) {
        uint32_t word = program[pc];
        uint32_t mask = OBFH_VM32_MASK(key, pc);
        asm volatile("eor %w0, %w1, %w2"
                     : "=&r"(decoded[pc]) : "r"(word), "r"(mask));
        if (decoded[pc] & UINT32_C(0xffffc000))
            return OBFH_VM32_INVALID_WORD;
    }
    if (decoded[0] != OBFH_VM32_PACK(OBFH_VM32_LOAD_A,0u,0u,0u) ||
        decoded[1] != OBFH_VM32_PACK(OBFH_VM32_LOAD_B,1u,0u,0u) ||
        decoded[3] != OBFH_VM32_PACK(OBFH_VM32_RETURN,0u,2u,0u))
        return OBFH_VM32_INVALID_WORD;
    if (decoded[2] != OBFH_VM32_PACK(expected_operation,2u,0u,1u))
        return OBFH_VM32_WRONG_OPERATION;

    /* Second pass interprets the verified bytecode. No fallback to
     * unprotected C bitwise arithmetic or a second VM is permitted.
     */
    for (pc = 0u; pc < OBFH_VM32_PROGRAM_WORDS; ++pc) {
        uint32_t word = decoded[pc];
        unsigned int op = (unsigned int)(word & 255u);
        unsigned int dst = (unsigned int)((word >> 8u) & 3u);
        unsigned int a = (unsigned int)((word >> 10u) & 3u);
        unsigned int b = (unsigned int)((word >> 12u) & 3u);
        uint32_t value;
        switch (op) {
        case OBFH_VM32_LOAD_A:
            registers[dst] = input_a;
            break;
        case OBFH_VM32_LOAD_B:
            registers[dst] = input_b;
            break;
        case OBFH_VM32_AND:
            asm volatile("and %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(registers[b]));
            registers[dst] = value;
            break;
        case OBFH_VM32_OR:
            asm volatile("orr %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(registers[b]));
            registers[dst] = value;
            break;
        case OBFH_VM32_XOR:
            asm volatile("eor %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(registers[b]));
            registers[dst] = value;
            break;
        case OBFH_VM32_NOT: {
            const uint32_t all_ones = UINT32_MAX;
            asm volatile("eor %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(all_ones));
            registers[dst] = value;
            break;
        }
        case OBFH_VM32_SHL: {
            uint32_t count = registers[b] & 31u;
            asm volatile("lsl %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(count));
            registers[dst] = value;
            break;
        }
        case OBFH_VM32_SHR: {
            uint32_t count = registers[b] & 31u;
            asm volatile("lsr %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(count));
            registers[dst] = value;
            break;
        }
        case OBFH_VM32_RETURN:
            *output = registers[a];
            *executed_steps = OBFH_VM32_PROGRAM_WORDS;
            return OBFH_VM32_OK;
        default:
            return OBFH_VM32_INVALID_WORD;
        }
    }
    return OBFH_VM32_INVALID_WORD;
}

/* Each source site owns a distinct immutable encoded stream. C GNU statement
 * expressions are supported by the pinned native and Windows ARM64 TinyCC.
 * The caller's expressions evaluate once and only once. All corruption is
 * fail-closed: the source-signature has no error return, therefore abort.
 */
#define OBFH_VM32_RUN(operation,input_a,input_b) ({ \
    enum { __obfh_vm_site = __COUNTER__, \
           __obfh_vm_key = OBFH_VM32_KEY(__obfh_vm_site) }; \
    static const uint32_t __obfh_vm_code[OBFH_VM32_PROGRAM_WORDS] = { \
        OBFH_VM32_ENCODE(OBFH_VM32_PACK(OBFH_VM32_LOAD_A,0u,0u,0u),__obfh_vm_key,0u), \
        OBFH_VM32_ENCODE(OBFH_VM32_PACK(OBFH_VM32_LOAD_B,1u,0u,0u),__obfh_vm_key,1u), \
        OBFH_VM32_ENCODE(OBFH_VM32_PACK((operation),2u,0u,1u),__obfh_vm_key,2u), \
        OBFH_VM32_ENCODE(OBFH_VM32_PACK(OBFH_VM32_RETURN,0u,2u,0u),__obfh_vm_key,3u) \
    }; \
    uint32_t __obfh_vm_a = (uint32_t)(input_a); \
    uint32_t __obfh_vm_b = (uint32_t)(input_b); \
    uint32_t __obfh_vm_out = 0u, __obfh_vm_steps = 0u; \
    if (obfh_arm64_vm32_execute(__obfh_vm_code,OBFH_VM32_PROGRAM_WORDS, \
            __obfh_vm_key,(operation),__obfh_vm_a,__obfh_vm_b, \
            &__obfh_vm_out,&__obfh_vm_steps) != OBFH_VM32_OK || \
        __obfh_vm_steps != OBFH_VM32_PROGRAM_WORDS) abort(); \
    (unsigned int)__obfh_vm_out; \
})
#define VM_AND(a,b) OBFH_VM32_RUN(OBFH_VM32_AND,a,b)
#define VM_OR(a,b) OBFH_VM32_RUN(OBFH_VM32_OR,a,b)
#define VM_XOR(a,b) OBFH_VM32_RUN(OBFH_VM32_XOR,a,b)
#define VM_NOT(a) OBFH_VM32_RUN(OBFH_VM32_NOT,a,0u)
#define VM_SHL(a,b) OBFH_VM32_RUN(OBFH_VM32_SHL,a,b)
#define VM_SHR(a,b) OBFH_VM32_RUN(OBFH_VM32_SHR,a,b)
#endif
