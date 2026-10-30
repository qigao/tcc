/* SPDX-License-Identifier: MIT
 * Integrated original-format VM dispatcher differential:
 * upstream OBFH_V_PROGRAM_0..7 decoded into the SAME register/flag owner.
 * Exact source blob: DosX-dev/obfus.h 4c06fbc06f907afb3989e2851bed7583cfd0b6b0.
 * All eight source program shapes are tested here. Not Release-ready.
 */
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "obfh_arm64_upstream_program_reference.h"

static int call_qualified(const upstream_test_program *p,
                          long double a,long double b,unsigned int floating,
                          unsigned int sample_id) {
    obfh_arm64_upstream_value in_a,in_b;
    long double actual=-345.0L,expected;
    uint32_t steps=0xdecafbadu;
    obfh_uprt_trace route;
    uint32_t nonce=(sample_id+1u)*UINT32_C(0x1020305);
    if (obfh_arm64_upvalue_encode((const unsigned char *)&a,sizeof a,
            p->seed,17,(nonce<<1u)|(floating&1u),&in_a)!=OBFH_UPVALUE_OK ||
        obfh_arm64_upvalue_encode((const unsigned char *)&b,sizeof b,
            p->seed,41,((nonce+7u)<<1u)|(floating&1u),&in_b)!=OBFH_UPVALUE_OK)
        return 20;
    memset(&route,0xa5,sizeof route);
    {
        enum obfh_uprt_status status=obfh_arm64_upstream_execute(
            p->words,p->length,p->seed,p->key,
            &in_a,17,&in_b,41,128u,&actual,&steps,&route);
        if (status!=OBFH_UPRT_OK) {
            fprintf(stderr,"UPRT_RUNTIME_FAIL variant=%u operation=%u sample=%u status=%u\n",
                    p->variant,p->operation,sample_id,(unsigned int)status);
            return 21;
        }
    }
    expected=reference(p->operation,a,b,floating);
    if (!equal_number(actual,expected) || !expected_route(p,a,expected,&route) ||
        steps!=route.count) {
        fprintf(stderr,"UPRT_RUNTIME_DIFFERENTIAL variant=%u operation=%u sample=%u steps=%u traced=%u\n",
                p->variant,p->operation,sample_id,steps,route.count);
        return 22;
    }
    return 0;
}

static int variant_matrix(void) {
    static const long double numeric_a[]={
        -7.0L,0.0L,2.0L,3.5L,10.0L,1.0L,5.0L,0.0L,
        -0.0L,8.0L,13.0L,1.0L/1073741824.0L,
        (long double)INT_MIN
    };
    static const long double numeric_b[]={
        2.0L,0.0L,3.0L,-2.0L,3.0L,0.0L,1.25L,7.0L,
        0.0L,4.0L,3.0L,1.0L,-1.0L
    };
    unsigned int cfg,v,op,i,accepted=0u,physical_modes=0u;
    for (cfg=0u;cfg<8u;++cfg) {
        uint32_t seed=(uint32_t)OBFH_SEED ^ (cfg*UINT32_C(0x30455971));
        uint32_t key=(cfg<<29u)|((UINT32_C(0x5d721300)^seed)&UINT32_C(0x1fffffff));
        physical_modes|=1u<<((seed>>4u)%4u);
        for (v=0u;v<8u;++v)for (op=0u;op<=UOP_BSHR;++op) {
            upstream_test_program program;
            if (!build_program(&program,seed,key+(v*UINT32_C(3266489917)),v,op))
                return 30;
            for (i=0u;i<sizeof numeric_a/sizeof numeric_a[0];++i) {
                long double a=numeric_a[i],b=numeric_b[i];
                unsigned int floating=(cfg+i)&1u;
                int rc;
                /* Upstream bitwise source macros first convert args to
                 * unsigned. This lower-level fixture only passes defined
                 * unsigned-range operand representations. */
                if (op>=UOP_BAND &&
                    (a<0.0L || b<0.0L || a!=(long double)(uint32_t)a ||
                     b!=(long double)(uint32_t)b)) continue;
                if (op==UOP_MOD && (a!=(long double)(int)a ||
                                    b!=(long double)(int)b)) continue;
                if (op==UOP_DIV && floating && b==0.0L) continue;
                rc=call_qualified(&program,a,b,floating,cfg*100u+i);
                if (rc) return rc+50;
                ++accepted;
            }
            if (op>=UOP_EQ && op<=UOP_TRUTH) {
                /* The original COMPARE/TEST flags distinguish unordered
                 * NaN from ordered floating comparisons. */
                union { uint64_t bits; double number; } special;
                int rc;
                special.bits=UINT64_C(0x7ff8000000000001);
                rc=call_qualified(&program,(long double)special.number,
                                  2.0L,1u,cfg*4096u+v*256u+op);
                if (rc) return rc+90;
                ++accepted;
                special.bits=UINT64_C(0x7ff0000000000000);
                rc=call_qualified(&program,(long double)special.number,
                                  2.0L,1u,cfg*4096u+v*256u+op+1u);
                if (rc) return rc+91;
                ++accepted;
            }
        }
    }
    /* Every original physical layout must actually own the working state
     * between decoded instructions. No mode may be covered only by a
     * separate sizeof/offsetof or copy-only test. */
    if (accepted<5000u || physical_modes!=0x0fu) {
        fprintf(stderr,"UPRT_LIVE_CONTEXT_MATRIX_FAIL accepted=%u modes=%02x\n",
                accepted,physical_modes);
        return 81;
    }
    return 0;
}

static int rejection_matrix(void) {
    upstream_test_program p;
    obfh_arm64_upstream_value in_a,in_b;
    long double a=5.0L,b=3.0L,out;
    uint32_t steps;
    obfh_uprt_trace trace,sentinel;
    uint32_t seed=(uint32_t)OBFH_SEED,key=UINT32_C(0x5d721300)^seed;
    obfh_upstream_fields word;
    if (!build_program(&p,seed,key,0u,UOP_ADD) ||
        obfh_arm64_upvalue_encode((const unsigned char *)&a,sizeof a,
           seed,17,3u,&in_a)!=OBFH_UPVALUE_OK ||
        obfh_arm64_upvalue_encode((const unsigned char *)&b,sizeof b,
           seed,41,5u,&in_b)!=OBFH_UPVALUE_OK) return 90;
    memset(&sentinel,0x5c,sizeof sentinel);
#define BAD(prog,len,inA,saltA,inB,saltB,budget,expect) do { \
    enum obfh_uprt_status observed; \
    out=-999.0L;steps=0xdecafbadu;trace=sentinel; \
    observed=obfh_arm64_upstream_execute((prog),(len),seed,key, \
           (inA),(saltA),(inB),(saltB),(budget),&out,&steps,&trace); \
    if (observed!=(expect) || out!=-999.0L || steps!=0xdecafbadu || \
        memcmp(&trace,&sentinel,sizeof trace)!=0) { \
        fprintf(stderr,"UPRT_REJECT_FAIL line=%d got=%u expected=%u steps=%u\n", \
                __LINE__,(unsigned int)observed,(unsigned int)(expect),steps); \
        return 89; \
    } \
} while(0)
    BAD(p.words,0u,&in_a,17,&in_b,41,128u,OBFH_UPRT_INVALID_ARGS);
    BAD(p.words,33u,&in_a,17,&in_b,41,128u,OBFH_UPRT_INVALID_ARGS);
    BAD(p.words,p.length,&in_a,17,&in_b,41,0u,OBFH_UPRT_INVALID_ARGS);
    BAD(p.words,p.length,&in_a,17,&in_b,41,129u,OBFH_UPRT_INVALID_ARGS);
    BAD((const uint32_t *)0,p.length,&in_a,17,&in_b,41,128u,OBFH_UPRT_INVALID_ARGS);
    BAD(p.words,p.length,&in_a,15,&in_b,41,128u,OBFH_UPRT_INVALID_ARGS);
    BAD(p.words,p.length,&in_a,17,&in_b,49,128u,OBFH_UPRT_INVALID_ARGS);
    BAD(p.words,p.length,&in_a,17,&in_b,41,3u,OBFH_UPRT_BUDGET);
    in_a.floating=2u;
    BAD(p.words,p.length,&in_a,17,&in_b,41,128u,OBFH_UPRT_INVALID_ARGS);
    in_a.floating=1u;
    if (!obfh_upstream_decode(seed,key,3u,p.words[3],&word)) return 91;
    word.opcode=27u;
    if (!obfh_upstream_encode(seed,key,3u,&word,&p.words[3])) return 92;
    BAD(p.words,p.length,&in_a,17,&in_b,41,128u,OBFH_UPRT_INVALID_PROGRAM);
    if (!build_program(&p,seed,key,0u,UOP_ADD)) return 93;
    if (!obfh_upstream_decode(seed,key,3u,p.words[3],&word)) return 94;
    /* The fourth register is seed/key-selected, not hardcoded R3. */
    word.a=p.vd; /* remains uninitialized in original variant 0 */
    if (!obfh_upstream_encode(seed,key,3u,&word,&p.words[3])) return 95;
    BAD(p.words,p.length,&in_a,17,&in_b,41,128u,OBFH_UPRT_UNINITIALIZED);
    if (!build_program(&p,seed,key,0u,UOP_EQ)) return 96;
    if (!obfh_upstream_decode(seed,key,2u,p.words[2],&word)) return 97;
    word.immediate=7u;
    if (!obfh_upstream_encode(seed,key,2u,&word,&p.words[2])) return 98;
    BAD(p.words,p.length,&in_a,17,&in_b,41,128u,OBFH_UPRT_INVALID_PROGRAM);
    if (!build_program(&p,seed,key,0u,UOP_EQ)) return 99;
    if (!obfh_upstream_decode(seed,key,1u,p.words[1],&word)) return 100;
    word.opcode=OBFH_UPRT_JUMP;word.immediate=4u;
    if (!obfh_upstream_encode(seed,key,1u,&word,&p.words[1])) return 101;
    BAD(p.words,p.length,&in_a,17,&in_b,41,128u,OBFH_UPRT_INVALID_PROGRAM);
    if (!build_program(&p,seed,key,0u,UOP_EQ)) return 102;
    if (!obfh_upstream_decode(seed,key,1u,p.words[1],&word)) return 103;
    word.opcode=OBFH_UPRT_JUMP;word.immediate=1u;
    if (!obfh_upstream_encode(seed,key,1u,&word,&p.words[1])) return 104;
    BAD(p.words,p.length,&in_a,17,&in_b,41,4u,OBFH_UPRT_BUDGET);
    if (!build_program(&p,seed,key,0u,UOP_ADD)) return 105;
    /* Bad input range on a bitwise ALU must never invoke undefined casts. */
    if (!build_program(&p,seed,key,0u,UOP_BAND)) return 106;
    a=-1.0L;
    if (obfh_arm64_upvalue_encode((const unsigned char *)&a,sizeof a,
        seed,17,1u,&in_a)!=OBFH_UPVALUE_OK) return 107;
    BAD(p.words,p.length,&in_a,17,&in_b,41,128u,OBFH_UPRT_NUMERIC_RANGE);
    a=5.0L;
    if (obfh_arm64_upvalue_encode((const unsigned char *)&a,sizeof a,
        seed,17,1u,&in_a)!=OBFH_UPVALUE_OK) return 108;
    if (!build_program(&p,seed,key,6u,UOP_ADD)) return 109;
    /* Original variant-6 CONTROL loop requires ten decoded steps. */
    BAD(p.words,p.length,&in_a,17,&in_b,41,9u,OBFH_UPRT_BUDGET);
    if (!build_program(&p,seed,key,7u,UOP_ADD)) return 110;
    if (!obfh_upstream_decode(seed,key,11u,p.words[11],&word)) return 111;
    word.d=3u;word.a=1u; /* selector 3 | (1 << 2) = 7, invalid */
    if (!obfh_upstream_encode(seed,key,11u,&word,&p.words[11])) return 112;
    BAD(p.words,p.length,&in_a,17,&in_b,41,128u,OBFH_UPRT_INVALID_PROGRAM);
    if (!build_program(&p,seed,key,7u,UOP_ADD)) return 113;
    if (!obfh_upstream_decode(seed,key,11u,p.words[11],&word)) return 114;
    word.immediate=(uint32_t)p.length; /* invalid source jump */
    if (!obfh_upstream_encode(seed,key,11u,&word,&p.words[11])) return 115;
    BAD(p.words,p.length,&in_a,17,&in_b,41,128u,OBFH_UPRT_INVALID_PROGRAM);
#undef BAD
    return 0;
}

int main(void) {
    int rc=variant_matrix();
    if (rc) return rc;
    return rejection_matrix();
}
