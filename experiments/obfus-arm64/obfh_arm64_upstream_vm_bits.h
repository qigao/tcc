/* SPDX-License-Identifier: MIT
 * Tiny, strict AArch64 bitwise executor for ORIGINAL obfus.h packed WORDS.
 * This consumes the upstream region 11/14 format via the exact codec.
 * It is NOT upstream Obfh_VirtualMachine: no long-double operands, nonce,
 * flags, variant programs, keyword rewriting, call/stack proxy or fallback.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_VM_BITS_H
#define QIGAO_OBFH_ARM64_UPSTREAM_VM_BITS_H
#include <stddef.h>
#include <stdint.h>
#include "obfh_arm64_upstream_codec.h"

/* Exact upstream instruction enum values (source blob 4c06fbc0). */
enum {
    OBFH_UPVM_LOAD_A = 1u,
    OBFH_UPVM_LOAD_B = 2u,
    OBFH_UPVM_BAND = 18u,
    OBFH_UPVM_BOR = 19u,
    OBFH_UPVM_BXOR = 20u,
    OBFH_UPVM_BNOT = 21u,
    OBFH_UPVM_BSHL = 22u,
    OBFH_UPVM_BSHR = 23u,
    OBFH_UPVM_RETURN = 26u
};
enum obfh_upstream_vm_status {
    OBFH_UPVM_OK = 0,
    OBFH_UPVM_INVALID_ARGS = 1,
    OBFH_UPVM_INVALID_PROGRAM = 2,
    OBFH_UPVM_WRONG_OPERATION = 3,
    OBFH_UPVM_BUDGET_EXCEEDED = 4
};
#define OBFH_UPVM_WORDS 4u
#define OBFH_UPVM_MAX_BUDGET 128u

/* The subset's precise register schedule:
 * pc0 LOAD_A -> R0, pc1 LOAD_B -> R1,
 * pc2 BAND/BOR/BXOR/BNOT/BSHL/BSHR R2 = R0 op R1,
 * pc3 RETURN R2. This schedule uses original opcode numbers and packed
 * stream format but does not claim original VM template equivalence.
 *
 * Two phases: decode and validate ALL words before touching caller state;
 * execute only after validation. An incorrect/corrupt seed or key may still
 * decode (upstream encoding has no authenticity), but unexpected opcode/
 * operand shape must fail; no unprotected C arithmetic fallback exists.
 */
static __attribute__((noinline)) enum obfh_upstream_vm_status
obfh_arm64_upstream_bitwise_execute(const uint32_t *program, size_t length,
                                    uint32_t seed, uint32_t key,
                                    unsigned int expected_opcode,
                                    uint32_t input_a, uint32_t input_b,
                                    unsigned int max_steps, uint32_t *output,
                                    uint32_t *executed_steps) {
    obfh_upstream_fields insn[OBFH_UPVM_WORDS];
    uint32_t r[3] = {0u,0u,0u};
    uint32_t result=0u;
    unsigned int pc;
    if (!program || !output || !executed_steps ||
        length != OBFH_UPVM_WORDS ||
        expected_opcode < OBFH_UPVM_BAND ||
        expected_opcode > OBFH_UPVM_BSHR ||
        !max_steps || max_steps > OBFH_UPVM_MAX_BUDGET)
        return OBFH_UPVM_INVALID_ARGS;

    for (pc=0u;pc<OBFH_UPVM_WORDS;++pc)
        if (!obfh_upstream_decode(seed,key,pc,program[pc],&insn[pc]))
            return OBFH_UPVM_INVALID_PROGRAM;
    if (insn[0].opcode != OBFH_UPVM_LOAD_A ||
        insn[0].d != 0u || insn[0].a != 0u || insn[0].b != 0u ||
        insn[0].immediate != 0u ||
        insn[1].opcode != OBFH_UPVM_LOAD_B ||
        insn[1].d != 1u || insn[1].a != 0u || insn[1].b != 0u ||
        insn[1].immediate != 0u ||
        insn[3].opcode != OBFH_UPVM_RETURN ||
        insn[3].d != 0u || insn[3].a != 2u || insn[3].b != 0u ||
        insn[3].immediate != 0u)
        return OBFH_UPVM_INVALID_PROGRAM;
    if (insn[2].opcode != expected_opcode ||
        insn[2].d != 2u || insn[2].a != 0u || insn[2].b != 1u ||
        insn[2].immediate != 0u)
        return OBFH_UPVM_WRONG_OPERATION;

    if (max_steps < OBFH_UPVM_WORDS)
        return OBFH_UPVM_BUDGET_EXCEEDED;

    /* Truly executes four decoded instructions, with A64 ALU opcodes.
     * Original upstream uses long-double encoded register storage; this
     * strict POC only handles the 32-bit unsigned bitwise sub-contract.
     */
    for (pc=0u;pc<OBFH_UPVM_WORDS;++pc) {
        switch (insn[pc].opcode) {
        case OBFH_UPVM_LOAD_A: r[0]=input_a; break;
        case OBFH_UPVM_LOAD_B: r[1]=input_b; break;
        case OBFH_UPVM_BAND:
            asm volatile("and %w0, %w1, %w2"
                         : "=&r"(result) : "r"(r[0]),"r"(r[1]));
            r[2]=result; break;
        case OBFH_UPVM_BOR:
            asm volatile("orr %w0, %w1, %w2"
                         : "=&r"(result) : "r"(r[0]),"r"(r[1]));
            r[2]=result; break;
        case OBFH_UPVM_BXOR:
            asm volatile("eor %w0, %w1, %w2"
                         : "=&r"(result) : "r"(r[0]),"r"(r[1]));
            r[2]=result; break;
        case OBFH_UPVM_BNOT: {
            const uint32_t ones = UINT32_MAX;
            asm volatile("eor %w0, %w1, %w2"
                         : "=&r"(result) : "r"(r[0]),"r"(ones));
            r[2]=result; break;
        }
        case OBFH_UPVM_BSHL: {
            uint32_t n=r[1]&31u;
            asm volatile("lsl %w0, %w1, %w2"
                         : "=&r"(result) : "r"(r[0]),"r"(n));
            r[2]=result; break;
        }
        case OBFH_UPVM_BSHR: {
            uint32_t n=r[1]&31u;
            asm volatile("lsr %w0, %w1, %w2"
                         : "=&r"(result) : "r"(r[0]),"r"(n));
            r[2]=result; break;
        }
        case OBFH_UPVM_RETURN:
            *output=r[2];
            *executed_steps=OBFH_UPVM_WORDS;
            return OBFH_UPVM_OK;
        default: return OBFH_UPVM_INVALID_PROGRAM;
        }
    }
    return OBFH_UPVM_INVALID_PROGRAM;
}
#endif
