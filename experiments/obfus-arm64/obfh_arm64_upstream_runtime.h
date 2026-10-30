/* SPDX-License-Identifier: MIT
 * Integrated, bounded AArch64 study of the ORIGINAL obfus.h instruction VM.
 * Upstream: DosX-dev/obfus.h Git blob 4c06fbc06f907afb3989e2851bed7583cfd0b6b0.
 * One execution owner: source-format packed codec + input byte transport +
 * four encoded long-double registers + flags/controls + native A64 bitwise.
 * No alternate interpreter, C plaintext fallback, code patching, or SP/LR edits.
 *
 * IMPORTANT: semantic study, NOT the original Windows API/ABI, exact 8-variant
 * source macro facility, trap-on-error process exit, or Release qualification.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_RUNTIME_H
#define QIGAO_OBFH_ARM64_UPSTREAM_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "obfh_arm64_upstream_codec.h"
#include "obfh_arm64_upstream_registers.h"

#define OBFH_UPRT_MAX_WORDS 32u
#define OBFH_UPRT_MAX_STEPS 128u

/* These are the exact upstream region-11 opcode numbers. The explicit prefix
 * prevents accidental interchange with the independent VM32 POC enum. */
enum obfh_uprt_opcode {
    OBFH_UPRT_LOAD_A=1, OBFH_UPRT_LOAD_B=2, OBFH_UPRT_MOVE=3,
    OBFH_UPRT_SWAP=4, OBFH_UPRT_ADD=5, OBFH_UPRT_SUB=6,
    OBFH_UPRT_MUL=7, OBFH_UPRT_DIV=8, OBFH_UPRT_MOD=9,
    OBFH_UPRT_COMPARE=10, OBFH_UPRT_BOOLEAN=11, OBFH_UPRT_TEST=12,
    OBFH_UPRT_CADD=13, OBFH_UPRT_CXOR=14, OBFH_UPRT_CROL=15,
    OBFH_UPRT_JUMP=16, OBFH_UPRT_JFLAG=17,
    OBFH_UPRT_BAND=18, OBFH_UPRT_BOR=19, OBFH_UPRT_BXOR=20,
    OBFH_UPRT_BNOT=21, OBFH_UPRT_BSHL=22, OBFH_UPRT_BSHR=23,
    OBFH_UPRT_LOAD_PAIR=24, OBFH_UPRT_COPY_RETURN=25, OBFH_UPRT_RETURN=26
};
enum obfh_uprt_flags {
    OBFH_UPRT_EQ=1u, OBFH_UPRT_LT=2u, OBFH_UPRT_GT=4u,
    OBFH_UPRT_UN=8u, OBFH_UPRT_TRUE=16u, OBFH_UPRT_CONTROL=32u
};
enum obfh_uprt_status {
    OBFH_UPRT_OK=0,
    OBFH_UPRT_INVALID_ARGS=1,
    OBFH_UPRT_INVALID_PROGRAM=2,
    OBFH_UPRT_UNINITIALIZED=3,
    OBFH_UPRT_INVALID_FLAGS=4,
    OBFH_UPRT_BUDGET=5,
    OBFH_UPRT_NO_RETURN=6,
    OBFH_UPRT_NUMERIC_RANGE=7
};
typedef struct {
    unsigned int count;
    unsigned char pc[OBFH_UPRT_MAX_STEPS];
    unsigned char opcode[OBFH_UPRT_MAX_STEPS];
} obfh_uprt_trace;

/* Implementations return status. Never touch user-owned result, step count or
 * trace unless RETURN/COPY_RETURN succeeds. This deliberate transactional
 * qualification surface differs from upstream's ExitProcess-on-error policy.
 */
static inline enum obfh_uprt_status
obfh_uprt_read(const obfh_upreg_state *state,unsigned int reg,long double *out) {
    unsigned char bytes[sizeof(long double)];
    if (obfh_upreg_read(state,reg,bytes,sizeof bytes)!=OBFH_UPREG_OK)
        return OBFH_UPRT_UNINITIALIZED;
    memcpy(out,bytes,sizeof bytes);
    return OBFH_UPRT_OK;
}
static inline enum obfh_uprt_status
obfh_uprt_write(obfh_upreg_state *state,unsigned int reg,long double value) {
    unsigned char bytes[sizeof(long double)];
    memcpy(bytes,&value,sizeof bytes);
    if (obfh_upreg_write(state,reg,bytes,sizeof bytes)!=OBFH_UPREG_OK)
        return OBFH_UPRT_INVALID_PROGRAM;
    return OBFH_UPRT_OK;
}
static inline enum obfh_uprt_status
obfh_uprt_load(obfh_upreg_state *state,unsigned int reg,
               const unsigned char *raw) {
    return obfh_upreg_write(state,reg,raw,sizeof(long double))==OBFH_UPREG_OK
        ? OBFH_UPRT_OK : OBFH_UPRT_INVALID_PROGRAM;
}

/* Only add limits where upstream's C casts would otherwise have undefined
 * behavior. Valid original input range behaves identically. */
static inline int obfh_uprt_signed_castable(long double value) {
    return value == value && value >= (long double)INT_MIN &&
           value <= (long double)INT_MAX;
}
static inline int obfh_uprt_unsigned_castable(long double value) {
    return value == value && value >= 0.0L &&
           value <= (long double)UINT_MAX;
}
static inline enum obfh_uprt_status
obfh_uprt_eval_bitwise(unsigned int opcode,long double left,long double right,
                        long double *out) {
    uint32_t a,b=0u,value;
    if (!obfh_uprt_unsigned_castable(left) ||
        (opcode!=OBFH_UPRT_BNOT && !obfh_uprt_unsigned_castable(right)))
        return OBFH_UPRT_NUMERIC_RANGE;
    a=(uint32_t)(unsigned int)left;
    if (opcode!=OBFH_UPRT_BNOT) b=(uint32_t)(unsigned int)right;
    switch (opcode) {
    case OBFH_UPRT_BAND:
        asm volatile("and %w0, %w1, %w2" : "=&r"(value) : "r"(a),"r"(b));break;
    case OBFH_UPRT_BOR:
        asm volatile("orr %w0, %w1, %w2" : "=&r"(value) : "r"(a),"r"(b));break;
    case OBFH_UPRT_BXOR:
        asm volatile("eor %w0, %w1, %w2" : "=&r"(value) : "r"(a),"r"(b));break;
    case OBFH_UPRT_BNOT: {
        const uint32_t ones=UINT32_MAX;
        asm volatile("eor %w0, %w1, %w2" : "=&r"(value) : "r"(a),"r"(ones));break;
    }
    case OBFH_UPRT_BSHL: {
        uint32_t n=b&31u;
        asm volatile("lsl %w0, %w1, %w2" : "=&r"(value) : "r"(a),"r"(n));break;
    }
    case OBFH_UPRT_BSHR: {
        uint32_t n=b&31u;
        asm volatile("lsr %w0, %w1, %w2" : "=&r"(value) : "r"(a),"r"(n));break;
    }
    default: return OBFH_UPRT_INVALID_PROGRAM;
    }
    *out=(long double)value;
    return OBFH_UPRT_OK;
}

/* Reversible packed words are NOT authenticated. Validate entire reachable
 * program format and mandatory operand bounds before executing anything. */
static __attribute__((noinline)) enum obfh_uprt_status
obfh_arm64_upstream_execute(const uint32_t *program,size_t length,
             uint32_t seed,uint32_t key,
             const obfh_arm64_upstream_value *input_a,int salt_a,
             const obfh_arm64_upstream_value *input_b,int salt_b,
             unsigned int budget,long double *output,uint32_t *out_steps,
             obfh_uprt_trace *out_trace) {
    obfh_upreg_state state;
    obfh_upstream_fields code[OBFH_UPRT_MAX_WORDS];
    unsigned char raw_a[sizeof(long double)],raw_b[sizeof(long double)];
    obfh_uprt_trace route;
    unsigned int pc,steps=0u;
    if (!program || !input_a || !input_b || !output || !out_steps ||
        !length || length>OBFH_UPRT_MAX_WORDS ||
        !budget || budget>OBFH_UPRT_MAX_STEPS) return OBFH_UPRT_INVALID_ARGS;
    if (obfh_arm64_upvalue_decode(input_a,seed,salt_a,raw_a,sizeof raw_a)!=OBFH_UPVALUE_OK ||
        obfh_arm64_upvalue_decode(input_b,seed,salt_b,raw_b,sizeof raw_b)!=OBFH_UPVALUE_OK)
        return OBFH_UPRT_INVALID_ARGS;
    for (pc=0u;pc<length;++pc) {
        obfh_upstream_fields f;
        if (!obfh_upstream_decode(seed,key,pc,program[pc],&f) ||
            f.opcode<OBFH_UPRT_LOAD_A || f.opcode>OBFH_UPRT_RETURN)
            return OBFH_UPRT_INVALID_PROGRAM;
        if ((f.opcode==OBFH_UPRT_JUMP || f.opcode==OBFH_UPRT_JFLAG) &&
            f.immediate>=length) return OBFH_UPRT_INVALID_PROGRAM;
        if (f.opcode==OBFH_UPRT_BOOLEAN && f.immediate>6u)
            return OBFH_UPRT_INVALID_PROGRAM;
        if (f.opcode==OBFH_UPRT_JFLAG && (f.d|(f.a<<2u))>5u)
            return OBFH_UPRT_INVALID_PROGRAM;
        if (f.opcode==OBFH_UPRT_LOAD_PAIR && f.d==f.a)
            return OBFH_UPRT_INVALID_PROGRAM;
        code[pc]=f;
    }
    (void)obfh_upreg_initialize(&state,seed,key);
    memset(&route,0,sizeof route);
    pc=0u;
    for (;;) {
        obfh_upstream_fields f;
        long double a=0.0L,b=0.0L,result=0.0L;
        uint32_t flagbits;
        enum obfh_uprt_status rc;
        if (steps>=budget) return OBFH_UPRT_BUDGET;
        if (pc>=length) return OBFH_UPRT_NO_RETURN;
        f=code[pc];
        route.pc[steps]=(unsigned char)pc;
        route.opcode[steps]=(unsigned char)f.opcode;
        ++steps;
        ++pc;
        state.steps=steps;
        switch (f.opcode) {
        case OBFH_UPRT_LOAD_A:
            rc=obfh_uprt_load(&state,f.d,raw_a);
            if (rc!=OBFH_UPRT_OK) return rc;
            break;
        case OBFH_UPRT_LOAD_B:
            rc=obfh_uprt_load(&state,f.d,raw_b);
            if (rc!=OBFH_UPRT_OK) return rc;
            break;
        case OBFH_UPRT_LOAD_PAIR:
            if (obfh_uprt_load(&state,f.d,raw_a)!=OBFH_UPRT_OK ||
                obfh_uprt_load(&state,f.a,raw_b)!=OBFH_UPRT_OK)
                return OBFH_UPRT_INVALID_PROGRAM;
            break;
        case OBFH_UPRT_MOVE:
            if (obfh_upreg_copy(&state,f.d,f.a)!=OBFH_UPREG_OK)
                return OBFH_UPRT_UNINITIALIZED;
            break;
        case OBFH_UPRT_SWAP:
            if (obfh_upreg_swap(&state,f.a,f.b)!=OBFH_UPREG_OK)
                return OBFH_UPRT_UNINITIALIZED;
            break;
        case OBFH_UPRT_ADD: case OBFH_UPRT_SUB: case OBFH_UPRT_MUL:
        case OBFH_UPRT_DIV: case OBFH_UPRT_MOD:
        case OBFH_UPRT_COMPARE: case OBFH_UPRT_BOOLEAN:
        case OBFH_UPRT_TEST:
        case OBFH_UPRT_BAND: case OBFH_UPRT_BOR: case OBFH_UPRT_BXOR:
        case OBFH_UPRT_BNOT: case OBFH_UPRT_BSHL: case OBFH_UPRT_BSHR:
            if (f.opcode!=OBFH_UPRT_BOOLEAN) {
                rc=obfh_uprt_read(&state,f.a,&a);
                if (rc!=OBFH_UPRT_OK) return rc;
            }
            if ((f.opcode>=OBFH_UPRT_ADD && f.opcode<=OBFH_UPRT_COMPARE) ||
                (f.opcode>=OBFH_UPRT_BAND && f.opcode<=OBFH_UPRT_BSHR &&
                 f.opcode!=OBFH_UPRT_BNOT)) {
                rc=obfh_uprt_read(&state,f.b,&b);
                if (rc!=OBFH_UPRT_OK) return rc;
            }
            if (f.opcode==OBFH_UPRT_COMPARE) {
                flagbits=a!=a || b!=b ? OBFH_UPRT_UN :
                          a==b ? OBFH_UPRT_EQ :
                          a<b ? OBFH_UPRT_LT : OBFH_UPRT_GT;
                (void)obfh_upreg_set_flags(&state,flagbits);
                state.flags_ready=1u;
                break;
            }
            if (f.opcode==OBFH_UPRT_TEST) {
                (void)obfh_upreg_get_flags(&state,&flagbits);
                flagbits=(flagbits&OBFH_UPRT_CONTROL) |
                          (a!=0.0L ? OBFH_UPRT_TRUE : 0u);
                (void)obfh_upreg_set_flags(&state,flagbits);
                state.flags_ready=1u;
                break;
            }
            if (f.opcode==OBFH_UPRT_BOOLEAN) {
                if (!state.flags_ready) return OBFH_UPRT_INVALID_FLAGS;
                (void)obfh_upreg_get_flags(&state,&flagbits);
                switch (f.immediate) {
                case 0u: result=(flagbits&OBFH_UPRT_EQ)!=0u;break;
                case 1u: result=(flagbits&OBFH_UPRT_EQ)==0u;break;
                case 2u: result=(flagbits&OBFH_UPRT_LT)!=0u;break;
                case 3u: result=(flagbits&OBFH_UPRT_GT)!=0u;break;
                case 4u: result=(flagbits&(OBFH_UPRT_LT|OBFH_UPRT_EQ))!=0u;break;
                case 5u: result=(flagbits&(OBFH_UPRT_GT|OBFH_UPRT_EQ))!=0u;break;
                default: result=(flagbits&OBFH_UPRT_TRUE)!=0u;break;
                }
            } else if (f.opcode>=OBFH_UPRT_BAND && f.opcode<=OBFH_UPRT_BSHR) {
                rc=obfh_uprt_eval_bitwise(f.opcode,a,b,&result);
                if (rc!=OBFH_UPRT_OK) return rc;
            } else {
                switch (f.opcode) {
                case OBFH_UPRT_ADD: result=a+b;break;
                case OBFH_UPRT_SUB: result=a-b;break;
                case OBFH_UPRT_MUL: result=a*b;break;
                case OBFH_UPRT_DIV:
                    result=input_a->floating || b!=0.0L ? a/b : 0.0L;break;
                case OBFH_UPRT_MOD: {
                    int x,y;
                    if (!obfh_uprt_signed_castable(a) ||
                        !obfh_uprt_signed_castable(b))
                        return OBFH_UPRT_NUMERIC_RANGE;
                    x=(int)a;y=(int)b;
                    result=(long double)((y!=0 && !(x==INT_MIN && y==-1))
                                     ? x%y : 0);
                    break;
                }
                default: return OBFH_UPRT_INVALID_PROGRAM;
                }
            }
            if (obfh_uprt_write(&state,f.d,result)!=OBFH_UPRT_OK)
                return OBFH_UPRT_INVALID_PROGRAM;
            break;
        case OBFH_UPRT_CADD:
            if (obfh_upreg_cadd(&state,f.d,f.immediate)!=OBFH_UPREG_OK)
                return OBFH_UPRT_INVALID_PROGRAM;
            break;
        case OBFH_UPRT_CXOR:
            if (obfh_upreg_cxor(&state,f.d,f.immediate)!=OBFH_UPREG_OK)
                return OBFH_UPRT_INVALID_PROGRAM;
            break;
        case OBFH_UPRT_CROL:
            if (obfh_upreg_crol(&state,f.d,f.immediate)!=OBFH_UPREG_OK)
                return OBFH_UPRT_INVALID_PROGRAM;
            break;
        case OBFH_UPRT_JUMP: pc=f.immediate;break;
        case OBFH_UPRT_JFLAG: {
            unsigned int selector=f.d|(f.a<<2u);
            if (!state.flags_ready || selector>5u)
                return OBFH_UPRT_INVALID_FLAGS;
            (void)obfh_upreg_get_flags(&state,&flagbits);
            if (flagbits & (1u<<selector)) pc=f.immediate;
            break;
        }
        case OBFH_UPRT_COPY_RETURN:
            if (obfh_upreg_copy(&state,f.d,f.a)!=OBFH_UPREG_OK)
                return OBFH_UPRT_UNINITIALIZED;
            /* FALLTHROUGH: encoded source register copy, return dst. */
            rc=obfh_uprt_read(&state,f.d,&result);
            if (rc!=OBFH_UPRT_OK) return rc;
            *output=result;
            *out_steps=steps;
            if (out_trace) {
                route.count=steps;
                *out_trace=route;
            }
            return OBFH_UPRT_OK;
        case OBFH_UPRT_RETURN:
            rc=obfh_uprt_read(&state,f.a,&result);
            if (rc!=OBFH_UPRT_OK) return rc;
            *output=result;
            *out_steps=steps;
            if (out_trace) {
                route.count=steps;
                *out_trace=route;
            }
            return OBFH_UPRT_OK;
        default:return OBFH_UPRT_INVALID_PROGRAM;
        }
    }
}
#endif
