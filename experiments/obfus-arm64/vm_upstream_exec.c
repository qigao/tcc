/* SPDX-License-Identifier: MIT
 * Native A64 execution of an original obfus.h packed-word BITWISE subset.
 * Full long-double VM and original source-template variants remain unported.
 */
#include <stdint.h>
#include "obfh_arm64_upstream_vm_bits.h"
static volatile uint32_t compiled_build_seed = (uint32_t)OBFH_SEED;

static int make_program(uint32_t seed, uint32_t key, uint32_t op,
                        uint32_t program[OBFH_UPVM_WORDS]) {
    obfh_upstream_fields fields[OBFH_UPVM_WORDS] = {
        {OBFH_UPVM_LOAD_A,0u,0u,0u,0u},
        {OBFH_UPVM_LOAD_B,1u,0u,0u,0u},
        {0u,2u,0u,1u,0u},
        {OBFH_UPVM_RETURN,0u,2u,0u,0u}
    };
    unsigned int i;
    fields[2].opcode=op;
    for (i=0u;i<OBFH_UPVM_WORDS;++i)
        if (!obfh_upstream_encode(seed,key,i,&fields[i],&program[i]))
            return 0;
    return 1;
}
static uint32_t reference_operation(uint32_t op,uint32_t a,uint32_t b) {
    switch (op) {
    case OBFH_UPVM_BAND: return a&b;
    case OBFH_UPVM_BOR: return a|b;
    case OBFH_UPVM_BXOR: return a^b;
    case OBFH_UPVM_BNOT: return ~a;
    case OBFH_UPVM_BSHL: return a<<(b&31u);
    default: return a>>(b&31u);
    }
}
static int cross_matrix(void) {
    static const uint32_t bits[7] = {
        0u,1u,31u,32u,0x55555555u,0x80000000u,0xffffffffu
    };
    unsigned int opmode,wordmode,perm,layout,operation,i;
    uint32_t seed,key,program[OBFH_UPVM_WORDS];
    unsigned int profiles=0u,decisions=0u;
    for (opmode=0u;opmode<6u;++opmode)
        for (wordmode=0u;wordmode<9u;++wordmode)
            for (perm=0u;perm<6u;++perm)
                for (layout=0u;layout<4u;++layout) {
                    uint32_t subgroup=(perm+6u-(opmode*1024u)%6u)%6u;
                    seed=(opmode<<24u)|(subgroup<<14u)|(wordmode<<6u)|21u;
                    key=(layout<<29u)|
                         (((opmode+1u)*0x12345u+(wordmode+3u)*0x5b1u+
                           perm*0x7bdu)&0x1fffffffu);
                    for (operation=OBFH_UPVM_BAND;operation<=OBFH_UPVM_BSHR;++operation) {
                        if (!make_program(seed,key,operation,program)) return 10;
                        for (i=0u;i<7u;++i) {
                            uint32_t a=bits[i], b=bits[(i+perm+wordmode)%7u];
                            uint32_t output=0xcafebabeu, steps=0xdecafbadu;
                            uint32_t expected=reference_operation(operation,a,b);
                            if (obfh_arm64_upstream_bitwise_execute(
                                   program,4u,seed,key,operation,a,b,4u,
                                   &output,&steps)!=OBFH_UPVM_OK ||
                                output!=expected || steps!=4u) return 11;
                            ++decisions;
                        }
                    }
                    ++profiles;
                }
    if (profiles!=1296u || decisions!=1296u*6u*7u) return 12;
    return 0;
}
static int rejection(void) {
    uint32_t seed=compiled_build_seed;
    uint32_t key=seed ^ UINT32_C(0x395175d7);
    uint32_t program[OBFH_UPVM_WORDS],result,steps;
    if (!make_program(seed,key,OBFH_UPVM_BXOR,program)) return 20;
#define BAD(code,length,profile,key_value,op,budget,wanted) do { \
    result=0xcafebabeu;steps=0xdecafbadu; \
    if (obfh_arm64_upstream_bitwise_execute((code),(length),(profile), \
        (key_value),(op),0xf0f0c0c0u,0x7b7b7b7bu,(budget), \
        &result,&steps)!=(wanted) || result!=0xcafebabeu || \
        steps!=0xdecafbadu) return __LINE__; \
} while(0)
    BAD(program,3u,seed,key,OBFH_UPVM_BXOR,4u,OBFH_UPVM_INVALID_ARGS);
    BAD(program,5u,seed,key,OBFH_UPVM_BXOR,4u,OBFH_UPVM_INVALID_ARGS);
    BAD(program,4u,seed,key,17u,4u,OBFH_UPVM_INVALID_ARGS);
    BAD(program,4u,seed,key,OBFH_UPVM_BXOR,0u,OBFH_UPVM_INVALID_ARGS);
    BAD(program,4u,seed,key,OBFH_UPVM_BXOR,129u,OBFH_UPVM_INVALID_ARGS);
    BAD((const uint32_t *)0,4u,seed,key,OBFH_UPVM_BXOR,4u,OBFH_UPVM_INVALID_ARGS);
    BAD(program,4u,seed,key,OBFH_UPVM_BAND,4u,OBFH_UPVM_WRONG_OPERATION);
    BAD(program,4u,seed,key,OBFH_UPVM_BXOR,3u,OBFH_UPVM_BUDGET_EXCEEDED);
    program[2]^=1u<<7;
    result=0xcafebabeu;steps=0xdecafbadu;
    if (obfh_arm64_upstream_bitwise_execute(program,4u,seed,key,
          OBFH_UPVM_BXOR,7u,3u,4u,&result,&steps)==OBFH_UPVM_OK ||
        result!=0xcafebabeu || steps!=0xdecafbadu) return 30;
    if (!make_program(seed,key,OBFH_UPVM_BXOR,program)) return 31;
    program[3]^=1u<<25;
    result=0xcafebabeu;steps=0xdecafbadu;
    if (obfh_arm64_upstream_bitwise_execute(program,4u,seed,key,
          OBFH_UPVM_BXOR,7u,3u,4u,&result,&steps)==OBFH_UPVM_OK ||
        result!=0xcafebabeu || steps!=0xdecafbadu) return 32;
    if (!make_program(seed,key,OBFH_UPVM_BXOR,program)) return 33;
    BAD(program,4u,seed,key^UINT32_C(0x55555555),OBFH_UPVM_BXOR,4u,
        OBFH_UPVM_INVALID_PROGRAM);
#undef BAD
    result=0xcafebabeu;steps=0xdecafbadu;
    if (obfh_arm64_upstream_bitwise_execute(program,4u,seed,key,
          OBFH_UPVM_BXOR,7u,3u,4u,(uint32_t *)0,&steps)!=OBFH_UPVM_INVALID_ARGS ||
        steps!=0xdecafbadu) return 34;
    if (obfh_arm64_upstream_bitwise_execute(program,4u,seed,key,
          OBFH_UPVM_BXOR,7u,3u,4u,&result,(uint32_t *)0)!=OBFH_UPVM_INVALID_ARGS ||
        result!=0xcafebabeu) return 35;
    return 0;
}
int main(void) {
    int e=cross_matrix();
    if (e) return e;
    return rejection();
}
