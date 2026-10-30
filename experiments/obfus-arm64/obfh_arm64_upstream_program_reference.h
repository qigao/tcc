/* SPDX-License-Identifier: MIT
 * One source-shaped reference builder for eight original VM program templates.
 * Shared by semantic and immutable-byte tests. Not an alternate interpreter.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_PROGRAM_REFERENCE_H
#define QIGAO_OBFH_ARM64_UPSTREAM_PROGRAM_REFERENCE_H
#include <stdint.h>
#include <stddef.h>
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

#endif
