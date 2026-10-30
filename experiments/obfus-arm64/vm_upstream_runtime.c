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
#include "obfh_arm64_upstream_runtime.h"

enum {
    UOP_ADD=0,UOP_SUB=1,UOP_MUL=2,UOP_DIV=3,UOP_MOD=4,
    UOP_EQ=5,UOP_NE=6,UOP_LT=7,UOP_GT=8,UOP_LE=9,UOP_GE=10,
    UOP_ID=11,UOP_TRUTH=12,UOP_BAND=13,UOP_BOR=14,
    UOP_BXOR=15,UOP_BNOT=16,UOP_BSHL=17,UOP_BSHR=18
};

typedef struct {
    uint32_t key,seed;
    unsigned int variant,operation,va,vb,vc,vd;
    size_t length;
    uint32_t words[15];
} upstream_test_program;

static unsigned int opcode0(unsigned int op) {
    if (op>=UOP_BAND) return OBFH_UPRT_BAND+(op-UOP_BAND);
    if (op<5u) return OBFH_UPRT_ADD+op;
    if (op<11u) return OBFH_UPRT_COMPARE;
    return op==UOP_ID ? OBFH_UPRT_MOVE : OBFH_UPRT_TEST;
}
static unsigned int opcode1(unsigned int op) {
    return (op>=5u && op<11u)||op==UOP_TRUTH
        ? OBFH_UPRT_BOOLEAN : OBFH_UPRT_CXOR;
}
static unsigned int imm1(unsigned int op) {
    return op==UOP_TRUTH ? 6u : (op>=5u ? op-5u : 0u);
}
static int emit(upstream_test_program *p, unsigned int pc,
                unsigned int opcode,unsigned int d,unsigned int a,
                unsigned int b,unsigned int imm) {
    obfh_upstream_fields word;
    if (pc>=15u) return 0;
    word.opcode=opcode;word.d=d;word.a=a;word.b=b;word.immediate=imm;
    return obfh_upstream_encode(p->seed,p->key,pc,&word,&p->words[pc]);
}
#define PUT(pc,op,d,a,b,imm) do { \
    if(!emit(p,(pc),(op),(d),(a),(b),(imm))) return 0; \
} while (0)

/* Exact upstream field ownership and program 0..7 instruction shapes.
 * The original header generates immutable per-site code from constants.
 * This test builder generates equivalent words via the approved codec to
 * separate interpreter semantics from compile-time template emission.
 */
static int build_program(upstream_test_program *p,uint32_t seed,
                         uint32_t key,unsigned int variant,unsigned int operation) {
    unsigned int va,vb,vc,vd,vb_offset,vc_pick,vc_offset;
    unsigned int op0,op1,im;
    if (!p || variant>7u || operation>UOP_BSHR) return 0;
    memset(p,0,sizeof *p);
    p->seed=seed;p->key=key;p->variant=variant;p->operation=operation;
    va=key&3u;
    vb_offset=1u+((key>>3u)%3u);
    vc_pick=1u+((key>>5u)&1u);
    vc_offset=vc_pick+(vc_pick>=vb_offset);
    vb=(va+vb_offset)&3u;
    vc=(va+vc_offset)&3u;
    vd=(va+6u-vb_offset-vc_offset)&3u;
    p->va=va;p->vb=vb;p->vc=vc;p->vd=vd;
    op0=opcode0(operation);op1=opcode1(operation);im=imm1(operation);
    if (variant==0u) {
        p->length=4u;
        PUT(0u,OBFH_UPRT_LOAD_PAIR,va,vb,0u,0u);
        PUT(1u,op0,vc,va,vb,0u);
        PUT(2u,op1,vc,va,0u,im);
        PUT(3u,OBFH_UPRT_RETURN,0u,vc,0u,0u);
    } else if (variant==1u) {
        p->length=6u;
        PUT(0u,(key&32u)?OBFH_UPRT_LOAD_A:OBFH_UPRT_LOAD_B,
            (key&32u)?va:vb,0u,0u,0u);
        PUT(1u,(key&32u)?OBFH_UPRT_LOAD_B:OBFH_UPRT_LOAD_A,
            (key&32u)?vb:va,0u,0u,0u);
        PUT(2u,OBFH_UPRT_MOVE,vd,va,0u,0u);
        PUT(3u,op0,vc,vd,vb,0u);
        PUT(4u,op1,vc,vd,0u,im);
        PUT(5u,OBFH_UPRT_COPY_RETURN,va,vc,0u,0u);
    } else if (variant==2u) {
        p->length=11u;
        PUT(0u,OBFH_UPRT_LOAD_PAIR,va,vb,0u,0u);
        PUT(1u,OBFH_UPRT_TEST,0u,va,0u,0u);
        PUT(2u,OBFH_UPRT_JFLAG,0u,1u,0u,7u);
        PUT(3u,op0,vc,va,vb,0u);
        PUT(4u,op1,vc,va,0u,im);
        PUT(5u,OBFH_UPRT_JUMP,0u,0u,0u,10u);
        PUT(6u,OBFH_UPRT_CROL,0u,0u,0u,3u);
        PUT(7u,op0,vd,va,vb,0u);
        PUT(8u,op1,vd,va,0u,im);
        PUT(9u,OBFH_UPRT_MOVE,vc,vd,0u,0u);
        PUT(10u,OBFH_UPRT_RETURN,0u,vc,0u,0u);
    } else if (variant==3u) {
        p->length=10u;
        PUT(0u,OBFH_UPRT_LOAD_PAIR,va,vb,0u,0u);
        PUT(1u,OBFH_UPRT_TEST,0u,va,0u,0u);
        PUT(2u,OBFH_UPRT_JFLAG,0u,1u,0u,7u);
        PUT(3u,op0,vc,va,vb,0u);
        PUT(4u,op1,vc,va,0u,im);
        PUT(5u,OBFH_UPRT_RETURN,0u,vc,0u,0u);
        PUT(6u,OBFH_UPRT_CXOR,0u,0u,0u,5u);
        PUT(7u,op0,vd,va,vb,0u);
        PUT(8u,op1,vd,va,0u,im);
        PUT(9u,OBFH_UPRT_RETURN,0u,vd,0u,0u);
    } else if (variant==4u) {
        /* JUMP-first predecessor graph with source-selected SWAP pair. */
        p->length=12u;
        PUT(0u,OBFH_UPRT_JUMP,0u,0u,0u,4u);
        PUT(1u,op0,vc,va,vb,0u);
        PUT(2u,op1,vc,va,0u,im);
        PUT(3u,OBFH_UPRT_RETURN,0u,vc,0u,0u);
        PUT(4u,OBFH_UPRT_LOAD_PAIR,va,vb,0u,0u);
        PUT(5u,OBFH_UPRT_TEST,0u,va,0u,0u);
        PUT(6u,OBFH_UPRT_JFLAG,0u,1u,0u,9u);
        PUT(7u,OBFH_UPRT_MOVE,vd,va,0u,0u);
        PUT(8u,OBFH_UPRT_JUMP,0u,0u,0u,1u);
        PUT(9u,OBFH_UPRT_SWAP,0u,va,vb,0u);
        PUT(10u,OBFH_UPRT_SWAP,0u,va,vb,0u);
        PUT(11u,OBFH_UPRT_JUMP,0u,0u,0u,1u);
    } else if (variant==5u) {
        p->length=11u;
        PUT(0u,OBFH_UPRT_LOAD_PAIR,va,vb,0u,0u);
        PUT(1u,OBFH_UPRT_SWAP,0u,va,vb,0u);
        PUT(2u,OBFH_UPRT_TEST,0u,vb,0u,0u);
        PUT(3u,OBFH_UPRT_JFLAG,0u,1u,0u,7u);
        PUT(4u,op0,vc,vb,va,0u);
        PUT(5u,op1,vc,vb,0u,im);
        PUT(6u,OBFH_UPRT_JUMP,0u,0u,0u,10u);
        PUT(7u,op0,vd,vb,va,0u);
        PUT(8u,op1,vd,vb,0u,im);
        PUT(9u,OBFH_UPRT_MOVE,vc,vd,0u,0u);
        PUT(10u,OBFH_UPRT_RETURN,0u,vc,0u,0u);
    } else if (variant==6u) {
        p->length=8u;
        PUT(0u,OBFH_UPRT_LOAD_PAIR,va,vb,0u,0u);
        PUT(1u,OBFH_UPRT_CADD,0u,0u,0u,2u);
        PUT(2u,OBFH_UPRT_CADD,0u,0u,0u,262143u);
        PUT(3u,OBFH_UPRT_JFLAG,1u,1u,0u,2u);
        PUT(4u,OBFH_UPRT_MOVE,vd,va,0u,0u);
        PUT(5u,op0,vc,vd,vb,0u);
        PUT(6u,op1,vc,vd,0u,im);
        PUT(7u,OBFH_UPRT_RETURN,0u,vc,0u,0u);
    } else {
        p->length=15u;
        PUT(0u,OBFH_UPRT_LOAD_PAIR,va,vb,0u,0u);
        PUT(1u,OBFH_UPRT_TEST,0u,va,0u,0u);
        PUT(2u,OBFH_UPRT_JFLAG,0u,1u,0u,6u);
        PUT(3u,(key&64u)?OBFH_UPRT_CROL:OBFH_UPRT_MOVE,
            (key&64u)?1u:vd,(key&64u)?0u:va,0u,(key&64u)?7u:0u);
        PUT(4u,(key&64u)?OBFH_UPRT_MOVE:OBFH_UPRT_CROL,
            (key&64u)?vd:1u,(key&64u)?va:0u,0u,(key&64u)?0u:7u);
        PUT(5u,OBFH_UPRT_JUMP,0u,0u,0u,8u);
        PUT(6u,OBFH_UPRT_MOVE,vd,va,0u,0u);
        PUT(7u,OBFH_UPRT_CXOR,1u,0u,0u,57u);
        PUT(8u,op0,vc,vd,vb,0u);
        PUT(9u,op1,vc,vd,0u,im);
        PUT(10u,OBFH_UPRT_TEST,0u,vc,0u,0u);
        PUT(11u,OBFH_UPRT_JFLAG,0u,1u,0u,14u);
        PUT(12u,OBFH_UPRT_MOVE,va,vc,0u,0u);
        PUT(13u,OBFH_UPRT_RETURN,0u,va,0u,0u);
        PUT(14u,OBFH_UPRT_RETURN,0u,vc,0u,0u);
    }
    return 1;
}
#undef PUT

static long double reference(unsigned int op,long double a,long double b,
                              unsigned int floating) {
    switch (op) {
    case UOP_ADD:return a+b;
    case UOP_SUB:return a-b;
    case UOP_MUL:return a*b;
    case UOP_DIV:return floating||b!=0.0L ? a/b : 0.0L;
    case UOP_MOD: {
        int x=(int)a,y=(int)b;
        return (long double)((y!=0 && !(x==INT_MIN&&y==-1))?x%y:0);
    }
    case UOP_EQ:return a==b;
    case UOP_NE:return a!=b;
    case UOP_LT:return a<b;
    case UOP_GT:return a>b;
    case UOP_LE:return a<=b;
    case UOP_GE:return a>=b;
    case UOP_ID:return a;
    case UOP_TRUTH:return a!=0.0L;
    case UOP_BAND:return (long double)((uint32_t)a&(uint32_t)b);
    case UOP_BOR:return (long double)((uint32_t)a|(uint32_t)b);
    case UOP_BXOR:return (long double)((uint32_t)a^(uint32_t)b);
    case UOP_BNOT:return (long double)(~(uint32_t)a);
    case UOP_BSHL:return (long double)((uint32_t)a<<((uint32_t)b&31u));
    default:return (long double)((uint32_t)a>>((uint32_t)b&31u));
    }
}
static int equal_number(long double x,long double y) {
    if (x!=x || y!=y) return x!=x && y!=y;
    if (x==0.0L && y==0.0L) {
        /* Preserve signed zero without depending on platform libm linkage. */
        int sign_x=(1.0L/x)>0.0L;
        int sign_y=(1.0L/y)>0.0L;
        return sign_x==sign_y;
    }
    return x==y;
}
/* Validate the source program's control-flow PC trace, not just its answer. */
static int expected_route(const upstream_test_program *program,
                          long double a,long double result,
                          const obfh_uprt_trace *route) {
    unsigned char expected[15];
    unsigned int used=0u,i;
#define ADD_PC(pos) do { expected[used++]=(unsigned char)(pos); } while(0)
    switch (program->variant) {
    case 0u: ADD_PC(0);ADD_PC(1);ADD_PC(2);ADD_PC(3);break;
    case 1u: ADD_PC(0);ADD_PC(1);ADD_PC(2);ADD_PC(3);
             ADD_PC(4);ADD_PC(5);break;
    case 2u:
        ADD_PC(0);ADD_PC(1);ADD_PC(2);
        if (a!=0.0L) { ADD_PC(7);ADD_PC(8);ADD_PC(9); }
        else { ADD_PC(3);ADD_PC(4);ADD_PC(5); }
        ADD_PC(10);
        break;
    case 3u:
        ADD_PC(0);ADD_PC(1);ADD_PC(2);
        if (a!=0.0L) { ADD_PC(7);ADD_PC(8);ADD_PC(9); }
        else { ADD_PC(3);ADD_PC(4);ADD_PC(5); }
        break;
    case 4u:
        ADD_PC(0);ADD_PC(4);ADD_PC(5);ADD_PC(6);
        if (a!=0.0L) { ADD_PC(9);ADD_PC(10);ADD_PC(11); }
        else { ADD_PC(7);ADD_PC(8); }
        ADD_PC(1);ADD_PC(2);ADD_PC(3);
        break;
    case 5u:
        ADD_PC(0);ADD_PC(1);ADD_PC(2);ADD_PC(3);
        if (a!=0.0L) { ADD_PC(7);ADD_PC(8);ADD_PC(9); }
        else { ADD_PC(4);ADD_PC(5);ADD_PC(6); }
        ADD_PC(10);
        break;
    case 6u:
        ADD_PC(0);ADD_PC(1);ADD_PC(2);ADD_PC(3);ADD_PC(2);
        ADD_PC(3);ADD_PC(4);ADD_PC(5);ADD_PC(6);ADD_PC(7);
        break;
    case 7u:
        ADD_PC(0);ADD_PC(1);ADD_PC(2);
        if (a!=0.0L) { ADD_PC(6);ADD_PC(7); }
        else { ADD_PC(3);ADD_PC(4);ADD_PC(5); }
        ADD_PC(8);ADD_PC(9);ADD_PC(10);ADD_PC(11);
        if (result!=0.0L) ADD_PC(14);
        else { ADD_PC(12);ADD_PC(13); }
        break;
    default:return 0;
    }
#undef ADD_PC
    if (route->count!=used) return 0;
    for (i=0u;i<used;++i) {
        obfh_upstream_fields actual;
        if (route->pc[i]!=expected[i] ||
            !obfh_upstream_decode(program->seed,program->key,
                expected[i],program->words[expected[i]],&actual) ||
            route->opcode[i]!=actual.opcode)
            return 0;
    }
    return 1;
}

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
    unsigned int cfg,v,op,i,accepted=0u;
    for (cfg=0u;cfg<8u;++cfg) {
        uint32_t seed=(uint32_t)OBFH_SEED ^ (cfg*UINT32_C(0x30455971));
        uint32_t key=(cfg<<29u)|((UINT32_C(0x5d721300)^seed)&UINT32_C(0x1fffffff));
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
    if (accepted<5000u) return 81;
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
