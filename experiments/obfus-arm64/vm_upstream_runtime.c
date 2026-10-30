/* SPDX-License-Identifier: MIT
 * Integrated original-format VM dispatcher differential:
 * upstream OBFH_V_PROGRAM_0..3 decoded into the SAME register/flag owner.
 * Exact source blob: DosX-dev/obfus.h 4c06fbc06f907afb3989e2851bed7583cfd0b6b0.
 * Only the four admitted program shapes are tested here. Not Release-ready.
 */
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
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

/* Exact upstream field ownership and program 0/1/2/3 instruction shapes.
 * The original header generates immutable per-site code from constants.
 * This test builder generates equivalent words via the approved codec to
 * separate interpreter semantics from compile-time template emission.
 */
static int build_program(upstream_test_program *p,uint32_t seed,
                         uint32_t key,unsigned int variant,unsigned int operation) {
    unsigned int va,vb,vc,vd,vb_offset,vc_pick,vc_offset;
    unsigned int op0,op1,im;
    if (!p || variant>3u || operation>UOP_BSHR) return 0;
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
    } else {
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
static int expected_route(unsigned int v,long double a,
                          const obfh_uprt_trace *route) {
    static const unsigned char paths[4][2][8] = {
        {{0,1,2,3},{0,1,2,3}},
        {{0,1,2,3,4,5},{0,1,2,3,4,5}},
        {{0,1,2,3,4,5,10},{0,1,2,7,8,9,10}},
        {{0,1,2,3,4,5},{0,1,2,7,8,9}}
    };
    static const unsigned char counts[4]={4,6,7,6};
    const unsigned char *expected=paths[v][a!=0.0L];
    unsigned int i;
    if (route->count!=counts[v]) return 0;
    for (i=0u;i<counts[v];++i)
        if (route->pc[i]!=expected[i]) return 0;
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
    if (obfh_arm64_upstream_execute(p->words,p->length,p->seed,p->key,
            &in_a,17,&in_b,41,128u,&actual,&steps,&route)!=OBFH_UPRT_OK)
        return 21;
    expected=reference(p->operation,a,b,floating);
    if (!equal_number(actual,expected) || !expected_route(p->variant,a,&route) ||
        steps!=route.count) return 22;
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
        for (v=0u;v<4u;++v)for (op=0u;op<=UOP_BSHR;++op) {
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
    if (accepted<2500u) return 81;
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
    out=-999.0L;steps=0xdecafbadu;trace=sentinel; \
    if (obfh_arm64_upstream_execute((prog),(len),seed,key, \
           (inA),(saltA),(inB),(saltB),(budget),&out,&steps,&trace)!=(expect) || \
        out!=-999.0L || steps!=0xdecafbadu || \
        memcmp(&trace,&sentinel,sizeof trace)!=0) return __LINE__; \
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
    word.a=3u; /* uninitialized R3 at source RETURN */
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
#undef BAD
    return 0;
}

int main(void) {
    int rc=variant_matrix();
    if (rc) return rc;
    return rejection_matrix();
}
