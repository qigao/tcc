/* SPDX-License-Identifier: MIT
 * Automatically derive a per-declaration original obfus.h key using
 * __COUNTER__, __LINE__ and the original three OBFH_JUNK_WORD draws.
 * Each site is a distinct static const instruction object. This is an
 * opt-in NAMED SITE DECLARATION, not yet an automatic VM_* expression macro.
 * Reference: DosX-dev/obfus.h Git blob 4c06fbc06f907afb3989e2851bed7583cfd0b6b0.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_AUTO_SITE_H
#define QIGAO_OBFH_ARM64_UPSTREAM_AUTO_SITE_H
#include "obfh_arm64_upstream_static.h"

/* This reproduces OBFH_DRAW16 and OBFH_JUNK_WORD's exact unsigned
 * preprocessor arithmetic. OBFH_MIX_A expands its value twice but the
 * macro argument is prescanned once: exactly THREE __COUNTER__ draws
 * follow the site counter, not six. This was independently checked with
 * the original macro definitions under a strict host C preprocessor.
 */
#define OBFH_AS_DRAW16(COUNT,LINE,SEED) \
    (((((uint32_t)(COUNT)) + (uint32_t)(LINE) + (uint32_t)(SEED)) * \
       UINT32_C(2654435761)) & UINT32_C(65535))
#define OBFH_AS_JUNK(D0,D1,D2,LINE,SEED) \
    ((uint32_t)(OBFH_AS_DRAW16(D0,LINE,SEED) * UINT32_C(2246822519)) ^ \
     ((uint32_t)OBFH_AS_DRAW16(D1,LINE,SEED) << 16u) ^ \
     (uint32_t)(OBFH_AS_DRAW16(D2,LINE,SEED) * UINT32_C(3266489917)))
#define OBFH_AS_MIX_A(VALUE) \
    (((uint32_t)(VALUE) ^ ((uint32_t)(VALUE) >> 16u)) * UINT32_C(2246822507))
#define OBFH_AS_LENGTH(V) \
    ((V)==0u ? 4u : (V)==1u ? 6u : (V)==2u ? 11u : (V)==3u ? 10u : \
     (V)==4u ? 12u : (V)==5u ? 11u : (V)==6u ? 8u : 15u)

/* Exact upstream RND(1,2147483647u), with 32-bit unsigned wrap before
 * modulo. This is compile-time entropy from __COUNTER__ + __LINE__ + seed,
 * NOT a runtime RNG. Two operand expansions consume TWO counters after the
 * four site/junk counters. The original RND domain includes 1..2147483647.
 */
/* TinyCC Windows constant folding may widen a uint32_t multiply before
 * modulo. Force source-equivalent modulo-2^32 wrap explicitly via a 64-bit
 * intermediate MASK, and only then perform the source's range modulo.
 * Every operation remains an integral constant expression (no runtime RNG).
 */
/* Shared source RND(min,max) contract for admitted ranges. The original
 * unsigned 32-bit product must wrap BEFORE division; a widened compiler
 * constant folder may otherwise generate a different operand profile.
 * Range and inputs are integral constant expressions, not runtime RNG.
 */
#define OBFH_AS_RND_RANGE(COUNT,LINE,SEED,MIN,MAX) \
    ((uint32_t)(MIN) + (uint32_t)( \
      (((((uint64_t)(uint32_t)(COUNT) + \
           (uint64_t)(uint32_t)(LINE) + \
           (uint64_t)(uint32_t)(SEED)) * UINT64_C(2654435761)) & \
           UINT64_C(0xffffffff)) % \
       ((uint64_t)(uint32_t)(MAX) - (uint64_t)(uint32_t)(MIN) + \
        UINT64_C(1)))))
#define OBFH_AS_RND31(COUNT,LINE,SEED) \
    OBFH_AS_RND_RANGE(COUNT,LINE,SEED,1u,UINT32_C(2147483647))
#define OBFH_AS_RND_SALT(COUNT,LINE,SEED) \
    OBFH_AS_RND_RANGE(COUNT,LINE,SEED,16u,48u)
#define OBFH_AS_DECLARE_NONCES(NAME) \
    enum { \
        NAME##_operand_counter_a=__COUNTER__, \
        NAME##_operand_counter_b=__COUNTER__, \
        NAME##_nonce_a=OBFH_AS_RND31(NAME##_operand_counter_a, \
                                      NAME##_line,OBFH_SEED), \
        NAME##_nonce_b=OBFH_AS_RND31(NAME##_operand_counter_b, \
                                      NAME##_line,OBFH_SEED) \
    }; \
    _Static_assert(NAME##_operand_counter_a==NAME##_draw2+1u && \
                   NAME##_operand_counter_b==NAME##_operand_counter_a+1u, \
                   "Original obfus.h operand counters are not consecutive"); \
    _Static_assert(NAME##_nonce_a>=1u && \
                   (uint32_t)NAME##_nonce_a<=UINT32_C(2147483647) && \
                   NAME##_nonce_b>=1u && \
                   (uint32_t)NAME##_nonce_b<=UINT32_C(2147483647), \
                   "Original obfus.h operand nonce domain violated")

/* Capture all original draws once, in-order. The encoded key is invariant
 * with respect to runtime arguments and references only compile-time enums.
 */
#define OBFH_AS_META(NAME,OP) \
    enum { \
        NAME##_line=__LINE__, \
        NAME##_site=__COUNTER__, \
        NAME##_draw0=__COUNTER__, \
        NAME##_draw1=__COUNTER__, \
        NAME##_draw2=__COUNTER__, \
        NAME##_junk=OBFH_AS_JUNK(NAME##_draw0,NAME##_draw1, \
                                  NAME##_draw2,NAME##_line,OBFH_SEED), \
        NAME##_derivedkey=OBFH_AS_MIX_A(NAME##_junk ^ NAME##_site ^ \
                                          (uint32_t)OBFH_SEED) \
    }; \
    _Static_assert(NAME##_draw0==NAME##_site+1u && \
                   NAME##_draw1==NAME##_site+2u && \
                   NAME##_draw2==NAME##_site+3u, \
                   "Original obfus site counter draws not consecutive"); \
    OBFH_SC_META(NAME,OBFH_SEED,NAME##_derivedkey,OP, \
                 ((NAME##_derivedkey>>24u)&7u), \
                 OBFH_AS_LENGTH((NAME##_derivedkey>>24u)&7u))

/* Exact original array lengths. As in upstream OBFH_V_PROGRAM, each
 * compile-time choice has its OWN array type and precise element count.
 * Named global sites use static-duration const compound literals; expression
 * sites use const block-duration compound literals, precisely as the upstream
 * GNU statement-expression implementation. There is no runtime encoder,
 * no zero padded tail and no alternate executor.
 */
#define OBFH_AC_VARIANT_0(NAME) ((const uint32_t[4u]) { \
    OBFH_SC_WORD(NAME,0u,OBFH_UPRT_LOAD_PAIR,NAME##_va,NAME##_vb,0u,0u), \
    OBFH_SC_WORD(NAME,1u,NAME##_op0,NAME##_vc,NAME##_va,NAME##_vb,0u), \
    OBFH_SC_WORD(NAME,2u,NAME##_op1,NAME##_vc,NAME##_va,0u,NAME##_imm), \
    OBFH_SC_WORD(NAME,3u,OBFH_UPRT_RETURN,0u,NAME##_vc,0u,0u) \
})

#define OBFH_AC_VARIANT_1(NAME) ((const uint32_t[6u]) { \
    OBFH_SC_WORD(NAME,0u,((NAME##_key&32u)?OBFH_UPRT_LOAD_A:OBFH_UPRT_LOAD_B),((NAME##_key&32u)?NAME##_va:NAME##_vb),0u,0u,0u), \
    OBFH_SC_WORD(NAME,1u,((NAME##_key&32u)?OBFH_UPRT_LOAD_B:OBFH_UPRT_LOAD_A),((NAME##_key&32u)?NAME##_vb:NAME##_va),0u,0u,0u), \
    OBFH_SC_WORD(NAME,2u,OBFH_UPRT_MOVE,NAME##_vd,NAME##_va,0u,0u), \
    OBFH_SC_WORD(NAME,3u,NAME##_op0,NAME##_vc,NAME##_vd,NAME##_vb,0u), \
    OBFH_SC_WORD(NAME,4u,NAME##_op1,NAME##_vc,NAME##_vd,0u,NAME##_imm), \
    OBFH_SC_WORD(NAME,5u,OBFH_UPRT_COPY_RETURN,NAME##_va,NAME##_vc,0u,0u) \
})

#define OBFH_AC_VARIANT_2(NAME) ((const uint32_t[11u]) { \
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
})

#define OBFH_AC_VARIANT_3(NAME) ((const uint32_t[10u]) { \
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
})

#define OBFH_AC_VARIANT_4(NAME) ((const uint32_t[12u]) { \
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
})

#define OBFH_AC_VARIANT_5(NAME) ((const uint32_t[11u]) { \
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
})

#define OBFH_AC_VARIANT_6(NAME) ((const uint32_t[8u]) { \
    OBFH_SC_WORD(NAME,0u,OBFH_UPRT_LOAD_PAIR,NAME##_va,NAME##_vb,0u,0u), \
    OBFH_SC_WORD(NAME,1u,OBFH_UPRT_CADD,0u,0u,0u,2u), \
    OBFH_SC_WORD(NAME,2u,OBFH_UPRT_CADD,0u,0u,0u,262143u), \
    OBFH_SC_WORD(NAME,3u,OBFH_UPRT_JFLAG,1u,1u,0u,2u), \
    OBFH_SC_WORD(NAME,4u,OBFH_UPRT_MOVE,NAME##_vd,NAME##_va,0u,0u), \
    OBFH_SC_WORD(NAME,5u,NAME##_op0,NAME##_vc,NAME##_vd,NAME##_vb,0u), \
    OBFH_SC_WORD(NAME,6u,NAME##_op1,NAME##_vc,NAME##_vd,0u,NAME##_imm), \
    OBFH_SC_WORD(NAME,7u,OBFH_UPRT_RETURN,0u,NAME##_vc,0u,0u) \
})

#define OBFH_AC_VARIANT_7(NAME) ((const uint32_t[15u]) { \
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
})

#define OBFH_AC_PROGRAM(NAME) \
    __builtin_choose_expr(NAME##_variant==0u, OBFH_AC_VARIANT_0(NAME), \
        __builtin_choose_expr(NAME##_variant==1u, OBFH_AC_VARIANT_1(NAME), \
        __builtin_choose_expr(NAME##_variant==2u, OBFH_AC_VARIANT_2(NAME), \
        __builtin_choose_expr(NAME##_variant==3u, OBFH_AC_VARIANT_3(NAME), \
        __builtin_choose_expr(NAME##_variant==4u, OBFH_AC_VARIANT_4(NAME), \
        __builtin_choose_expr(NAME##_variant==5u, OBFH_AC_VARIANT_5(NAME), \
        __builtin_choose_expr(NAME##_variant==6u, OBFH_AC_VARIANT_6(NAME), \
        OBFH_AC_VARIANT_7(NAME))))))))

/* sizeof examines the CHOSEN array type rather than a decayed pointer.
 * The compile-time assertion prevents a variant/length mismatch even when
 * the compiler optimizes away an unexecuted branch.
 */
#define OBFH_AC_ASSERT_LENGTH(NAME) \
    _Static_assert(sizeof(OBFH_AC_PROGRAM(NAME)) == \
                   (size_t)NAME##_length*sizeof(uint32_t), \
                   "Original VM source site physical array size mismatch")

/* Original VM_EXEC gets its source metadata as integral constants. That
 * portion is legal at file scope, without constructing any code object.
 */
#define OBFH_AC_DECLARE_META(NAME,OP) \
    OBFH_AS_META(NAME,OP); \
    OBFH_AC_ASSERT_LENGTH(NAME)

/* The selected exact-size const compound literal has block duration when
 * expanded inside the upstream GNU statement expression. TinyCC accepts this
 * original-standard constructor but rejects a static pointer initialized
 * from __builtin_choose_expr at file scope. NO 15-word fallback is allowed.
 *
 * For file-scope immutable arrays use the already qualified explicit
 * OBFH_SC_DECLARE_0..7 which create static const arrays of exact length.
 */
#define OBFH_AC_DECLARE_EXPR(NAME,OP) \
    OBFH_AC_DECLARE_META(NAME,OP); \
    const uint32_t *const NAME##_words = OBFH_AC_PROGRAM(NAME)

#endif
