/* SPDX-License-Identifier: MIT
 * Strictly bounded AArch64 encoded VM32 control-flow experiment.
 * This extends the POC with real decoded TEST, JFLAG, JUMP and DEC instructions.
 * The encoding and exact instruction shapes are NOT compatible with upstream
 * DosX-dev/obfus.h; there is no fallback interpreter or production API.
 */
#ifndef QIGAO_OBFH_ARM64_VM32_FLOW_H
#define QIGAO_OBFH_ARM64_VM32_FLOW_H
#include "obfh_arm64_vm32_codec.h"

#define OBFH_VM32_FLOW_MAX_WORDS 16u
#define OBFH_VM32_FLOW_MAX_STEPS 128u
enum {
    OBFH_VM32_FLOW_TEST = 10u,
    OBFH_VM32_FLOW_JFLAG = 11u,
    OBFH_VM32_FLOW_JUMP = 12u,
    OBFH_VM32_FLOW_DEC = 13u
};
enum obfh_vm32_flow_status {
    OBFH_VM32_FLOW_OK = 0,
    OBFH_VM32_FLOW_INVALID_ARGS = 1,
    OBFH_VM32_FLOW_INVALID_WORD = 2,
    OBFH_VM32_FLOW_UNINITIALIZED_REGISTER = 3,
    OBFH_VM32_FLOW_UNINITIALIZED_FLAG = 4,
    OBFH_VM32_FLOW_BUDGET_EXCEEDED = 5,
    OBFH_VM32_FLOW_MISSING_RETURN = 6
};

/* Four-byte layout is deliberately separate from the upstream format:
 * opcode[7:0], d[9:8], a[11:10], b[13:12], target[23:14].
 * Upper bits 24..31 are forbidden. Only jumps may use the target field.
 * Targets are absolute word indices, limited to the body [2, length-1].
 */
#define OBFH_VM32_FLOW_PACK(op,d,a,b,target) \
    (OBFH_VM32_PACK(op,d,a,b) | ((uint32_t)(target) << 14u))
#define OBFH_VM32_FLOW_WORD(word,key,index) \
    OBFH_VM32_ENCODE((word),(key),(index))

static __attribute__((noinline)) enum obfh_vm32_flow_status
obfh_arm64_vm32_flow_execute(const uint32_t *encoded, size_t length,
                             uint32_t key, uint32_t input_a, uint32_t input_b,
                             unsigned int max_steps, uint32_t *output,
                             uint32_t *executed_steps) {
    uint32_t words[OBFH_VM32_FLOW_MAX_WORDS];
    uint32_t registers[3] = {0u, 0u, 0u};
    uint32_t defined = 0u;
    uint32_t flag = 0u;
    unsigned int flag_valid = 0u;
    unsigned int pc, steps = 0u;

    if (!encoded || !output || !executed_steps ||
        length < 4u || length > OBFH_VM32_FLOW_MAX_WORDS ||
        !max_steps || max_steps > OBFH_VM32_FLOW_MAX_STEPS)
        return OBFH_VM32_FLOW_INVALID_ARGS;

    /* Validate the *entire* encoded stream, including unreachable words,
     * before interpreting or touching any caller-visible outputs. */
    for (pc = 0u; pc < length; ++pc) {
        uint32_t mask = OBFH_VM32_MASK(key,pc);
        uint32_t word;
        uint32_t op, d, a, b, target;
        asm volatile("eor %w0, %w1, %w2"
                     : "=&r"(word) : "r"(encoded[pc]), "r"(mask));
        words[pc] = word;
        if ((word & UINT32_C(0xff000000)) != 0u)
            return OBFH_VM32_FLOW_INVALID_WORD;
        op = word & 255u;
        d = (word >> 8u) & 3u;
        a = (word >> 10u) & 3u;
        b = (word >> 12u) & 3u;
        target = (word >> 14u) & 1023u;
        if (pc == 0u) {
            if (word != OBFH_VM32_PACK(OBFH_VM32_LOAD_A,0u,0u,0u))
                return OBFH_VM32_FLOW_INVALID_WORD;
            continue;
        }
        if (pc == 1u) {
            if (word != OBFH_VM32_PACK(OBFH_VM32_LOAD_B,1u,0u,0u))
                return OBFH_VM32_FLOW_INVALID_WORD;
            continue;
        }
        if (pc == length - 1u) {
            if (op != OBFH_VM32_RETURN || d != 0u || b != 0u ||
                a >= 3u || target != 0u)
                return OBFH_VM32_FLOW_INVALID_WORD;
            continue;
        }
        if (d >= 3u || a >= 3u || b >= 3u)
            return OBFH_VM32_FLOW_INVALID_WORD;
        switch (op) {
        case OBFH_VM32_AND:
        case OBFH_VM32_OR:
        case OBFH_VM32_XOR:
        case OBFH_VM32_NOT:
        case OBFH_VM32_SHL:
        case OBFH_VM32_SHR:
            if (target != 0u) return OBFH_VM32_FLOW_INVALID_WORD;
            break;
        case OBFH_VM32_FLOW_DEC:
            if (target != 0u || b != 0u || d != a)
                return OBFH_VM32_FLOW_INVALID_WORD;
            break;
        case OBFH_VM32_FLOW_TEST:
            if (target != 0u || d != 0u || b != 0u)
                return OBFH_VM32_FLOW_INVALID_WORD;
            break;
        case OBFH_VM32_FLOW_JFLAG:
        case OBFH_VM32_FLOW_JUMP:
            if (d != 0u || a != 0u || b != 0u ||
                target < 2u || target >= length)
                return OBFH_VM32_FLOW_INVALID_WORD;
            break;
        default:
            return OBFH_VM32_FLOW_INVALID_WORD;
        }
    }

    pc = 0u;
    while (pc < length) {
        uint32_t word, op, d, a, b, target, value;
        if (steps >= max_steps)
            return OBFH_VM32_FLOW_BUDGET_EXCEEDED;
        word = words[pc];
        op = word & 255u;
        d = (word >> 8u) & 3u;
        a = (word >> 10u) & 3u;
        b = (word >> 12u) & 3u;
        target = (word >> 14u) & 1023u;
        ++steps;
        if (op == OBFH_VM32_LOAD_A || op == OBFH_VM32_LOAD_B) {
            registers[d] = op == OBFH_VM32_LOAD_A ? input_a : input_b;
            defined |= 1u << d;
            ++pc;
            continue;
        }
        if (op == OBFH_VM32_FLOW_JUMP) {
            pc = target;
            continue;
        }
        if (op == OBFH_VM32_FLOW_JFLAG) {
            if (!flag_valid) return OBFH_VM32_FLOW_UNINITIALIZED_FLAG;
            pc = flag ? target : pc + 1u;
            continue;
        }
        if (op == OBFH_VM32_RETURN) {
            if (!(defined & (1u << a)))
                return OBFH_VM32_FLOW_UNINITIALIZED_REGISTER;
            *output = registers[a];
            *executed_steps = steps;
            return OBFH_VM32_FLOW_OK;
        }
        if (!(defined & (1u << a)))
            return OBFH_VM32_FLOW_UNINITIALIZED_REGISTER;
        if (op == OBFH_VM32_FLOW_TEST) {
            /* Execute a genuine A64 register operation before C truth testing.
             * Only TEST may define the condition read by JFLAG. */
            asm volatile("orr %w0, %w1, %w1"
                         : "=&r"(value) : "r"(registers[a]));
            flag = value != 0u;
            flag_valid = 1u;
            ++pc;
            continue;
        }
        if (op == OBFH_VM32_FLOW_DEC) {
            const uint32_t one = 1u;
            asm volatile("sub %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(one));
            registers[d] = value;
            defined |= 1u << d;
            ++pc;
            continue;
        }
        if (op != OBFH_VM32_NOT && !(defined & (1u << b)))
            return OBFH_VM32_FLOW_UNINITIALIZED_REGISTER;
        switch (op) {
        case OBFH_VM32_AND:
            asm volatile("and %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(registers[b]));
            break;
        case OBFH_VM32_OR:
            asm volatile("orr %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(registers[b]));
            break;
        case OBFH_VM32_XOR:
            asm volatile("eor %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(registers[b]));
            break;
        case OBFH_VM32_NOT: {
            const uint32_t ones = UINT32_MAX;
            asm volatile("eor %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(ones));
            break;
        }
        case OBFH_VM32_SHL: {
            const uint32_t shift = registers[b] & 31u;
            asm volatile("lsl %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(shift));
            break;
        }
        case OBFH_VM32_SHR: {
            const uint32_t shift = registers[b] & 31u;
            asm volatile("lsr %w0, %w1, %w2"
                         : "=&r"(value) : "r"(registers[a]), "r"(shift));
            break;
        }
        default:
            return OBFH_VM32_FLOW_INVALID_WORD;
        }
        registers[d] = value;
        defined |= 1u << d;
        ++pc;
    }
    return OBFH_VM32_FLOW_MISSING_RETURN;
}
#endif
