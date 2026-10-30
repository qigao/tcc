/* SPDX-License-Identifier: MIT
 * Explicit, per-site STATIC CONST program emission in the original obfus.h
 * 32-bit packed format. Reference: DosX-dev/obfus.h Git blob 4c06fbc0.
 * This is NOT yet automatic public VM_* / OBFH_VM_EXEC macro interception.
 * No executable memory, dynamic code patch, fallback interpreter or x86 asm.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_STATIC_H
#define QIGAO_OBFH_ARM64_UPSTREAM_STATIC_H
#include "obfh_arm64_upstream_runtime.h"

/* Compile-time integral constant expressions, intentionally mirror exact
 * source bit positions and permutations. Runtime codec remains the sole
 * decoder; independent source reference builder checks every emitted byte.
 */
#define OBFH_SC_LAYOUT(K) ((((uint32_t)(K)) >> 29u) & 3u)
#define OBFH_SC_MODE(S) ((((uint32_t)(S)) >> 24u) % 6u)
#define OBFH_SC_PERM(S) ((((uint32_t)(S)) >> 14u) % 6u)
#define OBFH_SC_MULT(K) (((((uint32_t)(K)) >> 8u) & 255u) | 1u)
#define OBFH_SC_SALT(K) (((uint32_t)(K)) & 255u)
#define OBFH_SC_OP(S,K,O) ((uint32_t)( \
    OBFH_SC_MODE(S)==0u ? (uint32_t)(O)*OBFH_SC_MULT(K)+OBFH_SC_SALT(K) : \
    OBFH_SC_MODE(S)==1u ? ((uint32_t)(O)*OBFH_SC_MULT(K))^OBFH_SC_SALT(K) : \
    OBFH_SC_MODE(S)==2u ? ((uint32_t)(O)^OBFH_SC_SALT(K))*OBFH_SC_MULT(K) : \
    OBFH_SC_MODE(S)==3u ? ((uint32_t)(O)+OBFH_SC_SALT(K))*OBFH_SC_MULT(K) : \
    OBFH_SC_MODE(S)==4u ? (uint32_t)(O)*OBFH_SC_MULT(K)-OBFH_SC_SALT(K) : \
                          ((uint32_t)(O)-OBFH_SC_SALT(K))*OBFH_SC_MULT(K)) & 255u)
#define OBFH_SC_D(S) (OBFH_SC_PERM(S)<2u ? 8u : OBFH_SC_PERM(S)<4u ? 10u : 12u)
#define OBFH_SC_A(S) (OBFH_SC_PERM(S)==0u || OBFH_SC_PERM(S)==5u ? 10u : \
                       OBFH_SC_PERM(S)<3u ? 12u : 8u)
#define OBFH_SC_B(S) (OBFH_SC_PERM(S)==0u || OBFH_SC_PERM(S)==3u ? 12u : \
                       OBFH_SC_PERM(S)==1u || OBFH_SC_PERM(S)==4u ? 10u : 8u)
#define OBFH_SC_SHIFT(K,POS) (OBFH_SC_LAYOUT(K)==1u ? (POS)+10u : \
    OBFH_SC_LAYOUT(K)==2u ? (POS)-8u : \
    OBFH_SC_LAYOUT(K)==3u ? ((POS)==8u ? 12u : (POS)-2u) : (POS))
#define OBFH_SC_OPSHIFT(K) (OBFH_SC_LAYOUT(K)==1u ? 24u : OBFH_SC_LAYOUT(K)==2u ? 6u : 0u)
#define OBFH_SC_IMMSHIFT(K) (OBFH_SC_LAYOUT(K)==1u ? 0u : 14u)
#define OBFH_SC_PACK(S,K,O,D,A,B,IMM) ((uint32_t)( \
      (OBFH_SC_OP(S,K,O) << OBFH_SC_OPSHIFT(K)) | \
      ((uint32_t)(D) << OBFH_SC_SHIFT(K,OBFH_SC_D(S))) | \
      ((uint32_t)(A) << OBFH_SC_SHIFT(K,OBFH_SC_A(S))) | \
      ((uint32_t)(B) << OBFH_SC_SHIFT(K,OBFH_SC_B(S))) | \
      (((uint32_t)(IMM) & UINT32_C(0x3ffff)) << OBFH_SC_IMMSHIFT(K))))
#define OBFH_SC_ROT(K) ((((uint32_t)(K)) % 31u) + 1u)
#define OBFH_SC_ROR(W,K) ((uint32_t)(((uint32_t)(W) >> OBFH_SC_ROT(K)) | \
                                   ((uint32_t)(W) << (32u-OBFH_SC_ROT(K)))))
#define OBFH_SC_WORDMODE(S) (((((uint32_t)(S)) >> 6u) & 15u) % 9u)
#define OBFH_SC_MASK(S,K,P) (OBFH_SC_WORDMODE(S)%3u==0u ? \
    ((uint32_t)(K)+(uint32_t)(P)*UINT32_C(0x9e3779b9)) : \
    OBFH_SC_WORDMODE(S)%3u==1u ? \
    ((uint32_t)(K)^((uint32_t)(P)*UINT32_C(0x9e3779b9))) : \
    ((uint32_t)(K)-(uint32_t)(P)*UINT32_C(0x9e3779b9)))
#define OBFH_SC_ENCODE(S,K,P,RAW) ((uint32_t)( \
    OBFH_SC_WORDMODE(S)/3u==0u ? ((uint32_t)(RAW)^OBFH_SC_MASK(S,K,P)) : \
    OBFH_SC_WORDMODE(S)/3u==1u ? ((uint32_t)(RAW)+OBFH_SC_MASK(S,K,P)) : \
                                 ((uint32_t)(RAW)-OBFH_SC_MASK(S,K,P))))
#define OBFH_SC_WORD(NAME,P,O,D,A,B,IMM) \
    OBFH_SC_ENCODE(NAME##_seed,NAME##_key,P, \
        OBFH_SC_ROR(OBFH_SC_PACK(NAME##_seed,NAME##_key,O,D,A,B,IMM),NAME##_key))

/* The enum-held key may be interpreted as signed by TinyCC when bit31 is
 * set. Upstream uses unsigned int for every right shift: cast before the
 * shift, not only before downstream bitwise packing. Otherwise the vb_offset
 * modulo-3 register permutation differs for high-bit keys. */
/* Explicit declaration is necessary to guarantee static object storage.
 * All site fields are C compile-time integral constants. The key must
 * encode the requested original 0..7 variant in bits 24..26.
 */
#define OBFH_SC_META(NAME,SEED,KEY,OP,VARIANT,LENGTH) \
    enum { \
      NAME##_seed=(SEED), NAME##_key=(KEY), NAME##_operation=(OP), \
      NAME##_variant=((uint32_t)NAME##_key>>24u)&7u, NAME##_length=(LENGTH), \
      NAME##_va=(uint32_t)NAME##_key&3u, \
      NAME##_vb_offset=1u+(((uint32_t)NAME##_key>>3u)%3u), \
      NAME##_vc_pick=1u+(((uint32_t)NAME##_key>>5u)&1u), \
      NAME##_vc_offset=NAME##_vc_pick+(NAME##_vc_pick>=NAME##_vb_offset), \
      NAME##_vb=(NAME##_va+NAME##_vb_offset)&3u, \
      NAME##_vc=(NAME##_va+NAME##_vc_offset)&3u, \
      NAME##_vd=(NAME##_va+6u-NAME##_vb_offset-NAME##_vc_offset)&3u, \
      NAME##_op0=NAME##_operation>=13u ? OBFH_UPRT_BAND+NAME##_operation-13u : \
                 NAME##_operation<5u ? OBFH_UPRT_ADD+NAME##_operation : \
                 NAME##_operation<11u ? OBFH_UPRT_COMPARE : \
                 NAME##_operation==11u ? OBFH_UPRT_MOVE : OBFH_UPRT_TEST, \
      NAME##_op1=((NAME##_operation>=5u && NAME##_operation<11u) || \
                   NAME##_operation==12u) ? OBFH_UPRT_BOOLEAN : OBFH_UPRT_CXOR, \
      NAME##_imm=NAME##_operation==12u ? 6u : \
                 NAME##_operation>=5u ? NAME##_operation-5u : 0u \
    }; \
    _Static_assert(NAME##_variant==(VARIANT), \
                   "Static obfus.h VM variant disagrees with key"); \
    _Static_assert(NAME##_operation<=18u, \
                   "Static obfus.h VM operation unsupported"); \
    _Static_assert(NAME##_length<=(unsigned)OBFH_UPRT_MAX_WORDS, \
                   "Static obfus.h VM program exceeds bounded owner")

#define OBFH_SC_DECLARE_0(NAME,SEED,KEY,OP) \
    OBFH_SC_META(NAME,SEED,KEY,OP,0u,4u); \
    static const uint32_t NAME##_words[4u] = { \
        OBFH_SC_WORD(NAME,0u,OBFH_UPRT_LOAD_PAIR,NAME##_va,NAME##_vb,0u,0u), \
        OBFH_SC_WORD(NAME,1u,NAME##_op0,NAME##_vc,NAME##_va,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,2u,NAME##_op1,NAME##_vc,NAME##_va,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,3u,OBFH_UPRT_RETURN,0u,NAME##_vc,0u,0u) \
    }

#define OBFH_SC_DECLARE_1(NAME,SEED,KEY,OP) \
    OBFH_SC_META(NAME,SEED,KEY,OP,1u,6u); \
    static const uint32_t NAME##_words[6u] = { \
        OBFH_SC_WORD(NAME,0u,((NAME##_key&32u)?OBFH_UPRT_LOAD_A:OBFH_UPRT_LOAD_B),((NAME##_key&32u)?NAME##_va:NAME##_vb),0u,0u,0u), \
        OBFH_SC_WORD(NAME,1u,((NAME##_key&32u)?OBFH_UPRT_LOAD_B:OBFH_UPRT_LOAD_A),((NAME##_key&32u)?NAME##_vb:NAME##_va),0u,0u,0u), \
        OBFH_SC_WORD(NAME,2u,OBFH_UPRT_MOVE,NAME##_vd,NAME##_va,0u,0u), \
        OBFH_SC_WORD(NAME,3u,NAME##_op0,NAME##_vc,NAME##_vd,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,4u,NAME##_op1,NAME##_vc,NAME##_vd,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,5u,OBFH_UPRT_COPY_RETURN,NAME##_va,NAME##_vc,0u,0u) \
    }

#define OBFH_SC_DECLARE_2(NAME,SEED,KEY,OP) \
    OBFH_SC_META(NAME,SEED,KEY,OP,2u,11u); \
    static const uint32_t NAME##_words[11u] = { \
        OBFH_SC_WORD(NAME,0u,OBFH_UPRT_LOAD_PAIR,NAME##_va,NAME##_vb,0u,0u), \
        OBFH_SC_WORD(NAME,1u,OBFH_UPRT_TEST,0u,NAME##_va,0u,0u), \
        OBFH_SC_WORD(NAME,2u,OBFH_UPRT_JFLAG,0u,1u,0u,7u), \
        OBFH_SC_WORD(NAME,3u,NAME##_op0,NAME##_vc,NAME##_va,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,4u,NAME##_op1,NAME##_vc,NAME##_va,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,5u,OBFH_UPRT_JUMP,0u,0u,0u,10u), \
        OBFH_SC_WORD(NAME,6u,OBFH_UPRT_CROL,0u,0u,0u,3u), \
        OBFH_SC_WORD(NAME,7u,NAME##_op0,NAME##_vd,NAME##_va,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,8u,NAME##_op1,NAME##_vd,NAME##_va,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,9u,OBFH_UPRT_MOVE,NAME##_vc,NAME##_vd,0u,0u), \
        OBFH_SC_WORD(NAME,10u,OBFH_UPRT_RETURN,0u,NAME##_vc,0u,0u) \
    }

#define OBFH_SC_DECLARE_3(NAME,SEED,KEY,OP) \
    OBFH_SC_META(NAME,SEED,KEY,OP,3u,10u); \
    static const uint32_t NAME##_words[10u] = { \
        OBFH_SC_WORD(NAME,0u,OBFH_UPRT_LOAD_PAIR,NAME##_va,NAME##_vb,0u,0u), \
        OBFH_SC_WORD(NAME,1u,OBFH_UPRT_TEST,0u,NAME##_va,0u,0u), \
        OBFH_SC_WORD(NAME,2u,OBFH_UPRT_JFLAG,0u,1u,0u,7u), \
        OBFH_SC_WORD(NAME,3u,NAME##_op0,NAME##_vc,NAME##_va,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,4u,NAME##_op1,NAME##_vc,NAME##_va,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,5u,OBFH_UPRT_RETURN,0u,NAME##_vc,0u,0u), \
        OBFH_SC_WORD(NAME,6u,OBFH_UPRT_CXOR,0u,0u,0u,5u), \
        OBFH_SC_WORD(NAME,7u,NAME##_op0,NAME##_vd,NAME##_va,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,8u,NAME##_op1,NAME##_vd,NAME##_va,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,9u,OBFH_UPRT_RETURN,0u,NAME##_vd,0u,0u) \
    }

#define OBFH_SC_DECLARE_4(NAME,SEED,KEY,OP) \
    OBFH_SC_META(NAME,SEED,KEY,OP,4u,12u); \
    static const uint32_t NAME##_words[12u] = { \
        OBFH_SC_WORD(NAME,0u,OBFH_UPRT_JUMP,0u,0u,0u,4u), \
        OBFH_SC_WORD(NAME,1u,NAME##_op0,NAME##_vc,NAME##_va,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,2u,NAME##_op1,NAME##_vc,NAME##_va,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,3u,OBFH_UPRT_RETURN,0u,NAME##_vc,0u,0u), \
        OBFH_SC_WORD(NAME,4u,OBFH_UPRT_LOAD_PAIR,NAME##_va,NAME##_vb,0u,0u), \
        OBFH_SC_WORD(NAME,5u,OBFH_UPRT_TEST,0u,NAME##_va,0u,0u), \
        OBFH_SC_WORD(NAME,6u,OBFH_UPRT_JFLAG,0u,1u,0u,9u), \
        OBFH_SC_WORD(NAME,7u,OBFH_UPRT_MOVE,NAME##_vd,NAME##_va,0u,0u), \
        OBFH_SC_WORD(NAME,8u,OBFH_UPRT_JUMP,0u,0u,0u,1u), \
        OBFH_SC_WORD(NAME,9u,OBFH_UPRT_SWAP,0u,NAME##_va,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,10u,OBFH_UPRT_SWAP,0u,NAME##_va,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,11u,OBFH_UPRT_JUMP,0u,0u,0u,1u) \
    }

#define OBFH_SC_DECLARE_5(NAME,SEED,KEY,OP) \
    OBFH_SC_META(NAME,SEED,KEY,OP,5u,11u); \
    static const uint32_t NAME##_words[11u] = { \
        OBFH_SC_WORD(NAME,0u,OBFH_UPRT_LOAD_PAIR,NAME##_va,NAME##_vb,0u,0u), \
        OBFH_SC_WORD(NAME,1u,OBFH_UPRT_SWAP,0u,NAME##_va,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,2u,OBFH_UPRT_TEST,0u,NAME##_vb,0u,0u), \
        OBFH_SC_WORD(NAME,3u,OBFH_UPRT_JFLAG,0u,1u,0u,7u), \
        OBFH_SC_WORD(NAME,4u,NAME##_op0,NAME##_vc,NAME##_vb,NAME##_va,0u), \
        OBFH_SC_WORD(NAME,5u,NAME##_op1,NAME##_vc,NAME##_vb,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,6u,OBFH_UPRT_JUMP,0u,0u,0u,10u), \
        OBFH_SC_WORD(NAME,7u,NAME##_op0,NAME##_vd,NAME##_vb,NAME##_va,0u), \
        OBFH_SC_WORD(NAME,8u,NAME##_op1,NAME##_vd,NAME##_vb,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,9u,OBFH_UPRT_MOVE,NAME##_vc,NAME##_vd,0u,0u), \
        OBFH_SC_WORD(NAME,10u,OBFH_UPRT_RETURN,0u,NAME##_vc,0u,0u) \
    }

#define OBFH_SC_DECLARE_6(NAME,SEED,KEY,OP) \
    OBFH_SC_META(NAME,SEED,KEY,OP,6u,8u); \
    static const uint32_t NAME##_words[8u] = { \
        OBFH_SC_WORD(NAME,0u,OBFH_UPRT_LOAD_PAIR,NAME##_va,NAME##_vb,0u,0u), \
        OBFH_SC_WORD(NAME,1u,OBFH_UPRT_CADD,0u,0u,0u,2u), \
        OBFH_SC_WORD(NAME,2u,OBFH_UPRT_CADD,0u,0u,0u,262143u), \
        OBFH_SC_WORD(NAME,3u,OBFH_UPRT_JFLAG,1u,1u,0u,2u), \
        OBFH_SC_WORD(NAME,4u,OBFH_UPRT_MOVE,NAME##_vd,NAME##_va,0u,0u), \
        OBFH_SC_WORD(NAME,5u,NAME##_op0,NAME##_vc,NAME##_vd,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,6u,NAME##_op1,NAME##_vc,NAME##_vd,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,7u,OBFH_UPRT_RETURN,0u,NAME##_vc,0u,0u) \
    }

#define OBFH_SC_DECLARE_7(NAME,SEED,KEY,OP) \
    OBFH_SC_META(NAME,SEED,KEY,OP,7u,15u); \
    static const uint32_t NAME##_words[15u] = { \
        OBFH_SC_WORD(NAME,0u,OBFH_UPRT_LOAD_PAIR,NAME##_va,NAME##_vb,0u,0u), \
        OBFH_SC_WORD(NAME,1u,OBFH_UPRT_TEST,0u,NAME##_va,0u,0u), \
        OBFH_SC_WORD(NAME,2u,OBFH_UPRT_JFLAG,0u,1u,0u,6u), \
        OBFH_SC_WORD(NAME,3u,((NAME##_key&64u)?OBFH_UPRT_CROL:OBFH_UPRT_MOVE),((NAME##_key&64u)?1u:NAME##_vd),((NAME##_key&64u)?0u:NAME##_va),0u,((NAME##_key&64u)?7u:0u)), \
        OBFH_SC_WORD(NAME,4u,((NAME##_key&64u)?OBFH_UPRT_MOVE:OBFH_UPRT_CROL),((NAME##_key&64u)?NAME##_vd:1u),((NAME##_key&64u)?NAME##_va:0u),0u,((NAME##_key&64u)?0u:7u)), \
        OBFH_SC_WORD(NAME,5u,OBFH_UPRT_JUMP,0u,0u,0u,8u), \
        OBFH_SC_WORD(NAME,6u,OBFH_UPRT_MOVE,NAME##_vd,NAME##_va,0u,0u), \
        OBFH_SC_WORD(NAME,7u,OBFH_UPRT_CXOR,1u,0u,0u,57u), \
        OBFH_SC_WORD(NAME,8u,NAME##_op0,NAME##_vc,NAME##_vd,NAME##_vb,0u), \
        OBFH_SC_WORD(NAME,9u,NAME##_op1,NAME##_vc,NAME##_vd,0u,NAME##_imm), \
        OBFH_SC_WORD(NAME,10u,OBFH_UPRT_TEST,0u,NAME##_vc,0u,0u), \
        OBFH_SC_WORD(NAME,11u,OBFH_UPRT_JFLAG,0u,1u,0u,14u), \
        OBFH_SC_WORD(NAME,12u,OBFH_UPRT_MOVE,NAME##_va,NAME##_vc,0u,0u), \
        OBFH_SC_WORD(NAME,13u,OBFH_UPRT_RETURN,0u,NAME##_va,0u,0u), \
        OBFH_SC_WORD(NAME,14u,OBFH_UPRT_RETURN,0u,NAME##_vc,0u,0u) \
    }

/* Original per-site CODE is a static const object, with no runtime encode.
 * Keep signature as a function-call expression for file-scope typeof checks.
 * Return status and outputs explicitly; no public VM_* alias or false parity.
 */
typedef struct {
    const uint32_t *words;
    size_t length;
    uint32_t seed, key;
    unsigned int variant, operation;
} obfh_sc_descriptor;
#define OBFH_SC_DESCRIPTOR(NAME) { \
    NAME##_words, NAME##_length, NAME##_seed, NAME##_key, \
    NAME##_variant, NAME##_operation }

/* A and B have SEPARATE original RND(1,2147483647u) draws.
 * Both nonces must be admitted before touching the output or execution
 * receipt. The current fixed salts 17/41 are separately qualified POC salts;
 * source-selected SALT_NUM1/SALT_NUM2 remain a distinct parity gate.
 */
static inline enum obfh_uprt_status obfh_sc_call_nonces(
        const obfh_sc_descriptor *site,
        long double left, long double right, unsigned int floating,
        uint32_t nonce_a, uint32_t nonce_b,
        long double *out, uint32_t *steps, obfh_uprt_trace *route) {
    obfh_arm64_upstream_value a,b;
    if (!site || !out || !steps ||
        site->length==0u || site->length>OBFH_UPRT_MAX_WORDS ||
        floating>1u ||
        nonce_a==0u || nonce_a>UINT32_C(2147483647) ||
        nonce_b==0u || nonce_b>UINT32_C(2147483647))
        return OBFH_UPRT_INVALID_ARGS;
    if (obfh_arm64_upvalue_encode((const unsigned char *)&left,sizeof left,
           site->seed,17,(nonce_a<<1u)|floating,&a)!=OBFH_UPVALUE_OK ||
        obfh_arm64_upvalue_encode((const unsigned char *)&right,sizeof right,
           site->seed,41,(nonce_b<<1u)|floating,&b)!=OBFH_UPVALUE_OK)
        return OBFH_UPRT_INVALID_ARGS;
    return obfh_arm64_upstream_execute(site->words,site->length,
          site->seed,site->key,&a,17,&b,41,128u,out,steps,route);
}
/* Keep the older independent nonce-fixture API for byte-contract tests;
 * its exact two-nonce values are now forwarded to the same execution owner.
 */
static inline enum obfh_uprt_status obfh_sc_call(
        const obfh_sc_descriptor *site,
        long double left, long double right, unsigned int floating,
        uint32_t nonce, long double *out, uint32_t *steps,
        obfh_uprt_trace *route) {
    return obfh_sc_call_nonces(site,left,right,floating,
                               nonce,nonce+7u,out,steps,route);
}
/* NAME must designate exactly one declared source site; arguments are
 * function operands and evaluated once each even with side effects.
 */
#define OBFH_SC_CALL(NAME,left,right,floating,nonce,out,steps,route) \
    obfh_sc_call(&(const obfh_sc_descriptor)OBFH_SC_DESCRIPTOR(NAME), \
                 (long double)(left),(long double)(right), \
                 (unsigned int)(floating),(uint32_t)(nonce),(out),(steps),(route))
#define OBFH_SC_CALL_NONCES(NAME,left,right,floating,na,nb,out,steps,route) \
    obfh_sc_call_nonces(&(const obfh_sc_descriptor)OBFH_SC_DESCRIPTOR(NAME), \
                 (long double)(left),(long double)(right), \
                 (unsigned int)(floating),(uint32_t)(na),(uint32_t)(nb), \
                 (out),(steps),(route))
#endif
