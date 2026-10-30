/* SPDX-License-Identifier: MIT
 * Native differential and negative admission of encoded A64 VM32 control flow.
 * Exercises a new POC format, not the full upstream obfus.h VM dispatcher.
 */
#include <stdint.h>
#include <stddef.h>
#include "obfh_arm64_vm32_flow.h"

#define FLOW_BRANCH_KEY OBFH_VM32_KEY(17u)
#define FLOW_LOOP_KEY OBFH_VM32_KEY(18u)
#define FW(op,d,a,b,t,key,pc) \
    OBFH_VM32_FLOW_WORD(OBFH_VM32_FLOW_PACK(op,d,a,b,t),key,pc)

/* If A is nonzero, compute A & B; otherwise, compute A ^ B.
 * Taken path: 0,1,2,3,6,7 = 6 steps.
 * Not-taken: 0,1,2,3,4,5,7 = 7 steps.
 */
static const uint32_t branch_program[8] = {
    FW(OBFH_VM32_LOAD_A,0,0,0,0,FLOW_BRANCH_KEY,0),
    FW(OBFH_VM32_LOAD_B,1,0,0,0,FLOW_BRANCH_KEY,1),
    FW(OBFH_VM32_FLOW_TEST,0,0,0,0,FLOW_BRANCH_KEY,2),
    FW(OBFH_VM32_FLOW_JFLAG,0,0,0,6,FLOW_BRANCH_KEY,3),
    FW(OBFH_VM32_XOR,2,0,1,0,FLOW_BRANCH_KEY,4),
    FW(OBFH_VM32_FLOW_JUMP,0,0,0,7,FLOW_BRANCH_KEY,5),
    FW(OBFH_VM32_AND,2,0,1,0,FLOW_BRANCH_KEY,6),
    FW(OBFH_VM32_RETURN,0,2,0,0,FLOW_BRANCH_KEY,7)
};

/* Count A down to zero through a backwards JFLAG. When A starts at zero,
 * unsigned wrap makes this run until the configured step budget is exhausted.
 */
static const uint32_t loop_program[6] = {
    FW(OBFH_VM32_LOAD_A,0,0,0,0,FLOW_LOOP_KEY,0),
    FW(OBFH_VM32_LOAD_B,1,0,0,0,FLOW_LOOP_KEY,1),
    FW(OBFH_VM32_FLOW_DEC,0,0,0,0,FLOW_LOOP_KEY,2),
    FW(OBFH_VM32_FLOW_TEST,0,0,0,0,FLOW_LOOP_KEY,3),
    FW(OBFH_VM32_FLOW_JFLAG,0,0,0,2,FLOW_LOOP_KEY,4),
    FW(OBFH_VM32_RETURN,0,0,0,0,FLOW_LOOP_KEY,5)
};

static int run_branch_cases(void) {
    uint32_t random = UINT32_C(0x91745123);
    unsigned int i;
    for (i = 0u; i < 4096u; ++i) {
        uint32_t a, b, output = 0x55555555u, steps = 0u;
        uint32_t expected;
        unsigned int expected_steps;
        random = random * 1664525u + 1013904223u;
        a = i % 17u == 0u ? 0u : random;
        if (i == 1u) a = UINT32_MAX;
        random = random * 1664525u + 1013904223u;
        b = random;
        expected = a ? (a & b) : (a ^ b);
        expected_steps = a ? 6u : 7u;
        if (obfh_arm64_vm32_flow_execute(branch_program,8u,FLOW_BRANCH_KEY,
               a,b,16u,&output,&steps) != OBFH_VM32_FLOW_OK ||
            output != expected || steps != expected_steps)
            return 10;
    }
    return 0;
}

static int run_loop_cases(void) {
    unsigned int n;
    for (n = 1u; n <= 41u; ++n) {
        uint32_t output = 0x12345678u, steps = 0u;
        unsigned int exact_budget = 3u * n + 3u;
        if (obfh_arm64_vm32_flow_execute(loop_program,6u,FLOW_LOOP_KEY,
                 n,0u,exact_budget,&output,&steps) != OBFH_VM32_FLOW_OK ||
            output != 0u || steps != exact_budget)
            return 11;
        output = 0xcafebabeu; steps = 0xdecafbadu;
        if (obfh_arm64_vm32_flow_execute(loop_program,6u,FLOW_LOOP_KEY,
                 n,0u,exact_budget-1u,&output,&steps) !=
                   OBFH_VM32_FLOW_BUDGET_EXCEEDED ||
            output != 0xcafebabeu || steps != 0xdecafbadu)
            return 12;
    }
    return 0;
}

/* Check that even an error *after* many executed instructions is atomic:
 * no partial outputs, statuses or execution counts reach the caller. */
static int run_rejections(void) {
    uint32_t mutable_program[8];
    uint32_t result, steps;
    unsigned int i;
#define BAD(code,length,key,a,b,budget,wanted) do { \
    result = 0xcafebabeu; steps = 0xdecafbadu; \
    if (obfh_arm64_vm32_flow_execute((code),(length),(key), \
          (a),(b),(budget),&result,&steps) != (wanted) || \
          result != 0xcafebabeu || steps != 0xdecafbadu) return __LINE__; \
} while(0)
    for (i=0u;i<8u;++i) mutable_program[i]=branch_program[i];

    BAD(mutable_program,3u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_ARGS);
    BAD(mutable_program,17u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_ARGS);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,2u,4u,0u,OBFH_VM32_FLOW_INVALID_ARGS);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,2u,4u,129u,OBFH_VM32_FLOW_INVALID_ARGS);
    BAD((const uint32_t *)0,8u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_ARGS);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY^UINT32_C(0x55555555),2u,4u,16u,OBFH_VM32_FLOW_INVALID_WORD);

    mutable_program[3] ^= 1u << 24;
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_WORD);
    mutable_program[3]=FW(OBFH_VM32_FLOW_JFLAG,0,0,0,8,FLOW_BRANCH_KEY,3);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_WORD);
    mutable_program[3]=FW(OBFH_VM32_FLOW_JFLAG,0,0,0,1,FLOW_BRANCH_KEY,3);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_WORD);
    mutable_program[3]=branch_program[3];

    mutable_program[6]=FW(OBFH_VM32_AND,3,0,1,0,FLOW_BRANCH_KEY,6);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_WORD);
    mutable_program[6]=FW(OBFH_VM32_AND,2,3,1,0,FLOW_BRANCH_KEY,6);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_WORD);
    mutable_program[6]=FW(OBFH_VM32_AND,2,0,1,1,FLOW_BRANCH_KEY,6);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_WORD);
    mutable_program[6]=FW(255,2,0,1,0,FLOW_BRANCH_KEY,6);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_WORD);
    mutable_program[6]=branch_program[6];

    mutable_program[7]=FW(OBFH_VM32_FLOW_JUMP,0,0,0,2,FLOW_BRANCH_KEY,7);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,2u,4u,16u,OBFH_VM32_FLOW_INVALID_WORD);
    mutable_program[7]=branch_program[7];

    mutable_program[2]=FW(OBFH_VM32_FLOW_JFLAG,0,0,0,7,FLOW_BRANCH_KEY,2);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,3u,4u,16u,OBFH_VM32_FLOW_UNINITIALIZED_FLAG);
    mutable_program[2]=FW(OBFH_VM32_FLOW_TEST,0,2,0,0,FLOW_BRANCH_KEY,2);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,3u,4u,16u,OBFH_VM32_FLOW_UNINITIALIZED_REGISTER);
    mutable_program[2]=branch_program[2];

    mutable_program[6]=FW(OBFH_VM32_AND,2,2,1,0,FLOW_BRANCH_KEY,6);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,3u,4u,16u,OBFH_VM32_FLOW_UNINITIALIZED_REGISTER);
    mutable_program[6]=branch_program[6];

    mutable_program[3]=FW(OBFH_VM32_FLOW_JFLAG,0,0,0,3,FLOW_BRANCH_KEY,3);
    BAD(mutable_program,8u,FLOW_BRANCH_KEY,3u,4u,16u,OBFH_VM32_FLOW_BUDGET_EXCEEDED);
    mutable_program[3]=branch_program[3];

    BAD(branch_program,8u,FLOW_BRANCH_KEY,0u,4u,6u,OBFH_VM32_FLOW_BUDGET_EXCEEDED);
    BAD(loop_program,6u,FLOW_LOOP_KEY,0u,0u,128u,OBFH_VM32_FLOW_BUDGET_EXCEEDED);
    BAD(loop_program,6u,FLOW_LOOP_KEY,42u,0u,128u,OBFH_VM32_FLOW_BUDGET_EXCEEDED);
#undef BAD
    result = 0xcafebabeu;steps = 0xdecafbadu;
    if (obfh_arm64_vm32_flow_execute(branch_program,8u,FLOW_BRANCH_KEY,
             2u,4u,16u,(uint32_t *)0,&steps) != OBFH_VM32_FLOW_INVALID_ARGS ||
        steps != 0xdecafbadu) return 60;
    if (obfh_arm64_vm32_flow_execute(branch_program,8u,FLOW_BRANCH_KEY,
             2u,4u,16u,&result,(uint32_t *)0) != OBFH_VM32_FLOW_INVALID_ARGS ||
        result != 0xcafebabeu) return 61;
    return 0;
}

int main(void) {
    int err = run_branch_cases();
    if (err) return err;
    err = run_loop_cases();
    if (err) return err;
    return run_rejections();
}
