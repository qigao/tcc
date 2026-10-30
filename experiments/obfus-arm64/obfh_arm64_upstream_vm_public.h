/* SPDX-License-Identifier: MIT
 * Experimental ORIGINAL-SPELLING VM_* expression markers for AArch64.
 * Uses the SAME original-format bytecode interpreter, with compile-time
 * __COUNTER__/line-derived key and a per-invocation static const program.
 * No plaintext C operation fallback. On execution rejection, abort().
 *
 * NOT a production drop-in obfus.h: original CRT/anti-debug/stack/keyword
 * interception, caller salt profiles, PE ABI/unwind, and exact-length
 * auto-site storage remain separate qualification gates.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_VM_PUBLIC_H
#define QIGAO_OBFH_ARM64_UPSTREAM_VM_PUBLIC_H
#include <stdlib.h>
#include "obfh_arm64_upstream_auto_site.h"

#if defined(VM_ADD) || defined(VM_SUB) || defined(VM_MUL) || defined(VM_DIV) || \
    defined(VM_MOD) || defined(VM_EQU) || defined(VM_NEQ) || \
    defined(VM_LSS) || defined(VM_GTR) || defined(VM_LEQ) || \
    defined(VM_GEQ) || defined(VM_AND) || defined(VM_OR) || \
    defined(VM_XOR) || defined(VM_NOT) || defined(VM_SHL) || \
    defined(VM_SHR) || defined(VM_IF) || defined(VM_ELSE_IF) || \
    defined(VM_ELSE)
#error "Original VM_* identifiers already defined: cannot silently replace them."
#endif
enum {
    OBFH_PUB_ADD=0, OBFH_PUB_SUB=1, OBFH_PUB_MUL=2,
    OBFH_PUB_DIV=3, OBFH_PUB_MOD=4, OBFH_PUB_EQ=5,
    OBFH_PUB_NE=6, OBFH_PUB_LT=7, OBFH_PUB_GT=8,
    OBFH_PUB_LE=9, OBFH_PUB_GE=10, OBFH_PUB_ID=11,
    OBFH_PUB_TRUTH=12, OBFH_PUB_BAND=13, OBFH_PUB_BOR=14,
    OBFH_PUB_BXOR=15, OBFH_PUB_BNOT=16,
    OBFH_PUB_BSHL=17, OBFH_PUB_BSHR=18
};
/* GNU statement expressions are used in upstream OBFH_VM_EXEC itself.
 * This public source form is therefore for code expressions INSIDE
 * functions. The earlier explicit OBFH_SC_CALL has a separate
 * file-scope typeof-compatible signature and remains available.
 */
#define OBFH_ARM64_VM_EXEC(OP,LEFT,RIGHT,FLOATING) ({ \
    OBFH_AS_DECLARE(__obfh_public_site,(OP)); \
    OBFH_AS_DECLARE_NONCES(__obfh_public_site); \
    long double __obfh_public_result=0.0L; \
    uint32_t __obfh_public_steps=0u; \
    enum obfh_uprt_status __obfh_public_status= \
        OBFH_SC_CALL_NONCES(__obfh_public_site,(LEFT),(RIGHT),(FLOATING), \
            __obfh_public_site_nonce_a,__obfh_public_site_nonce_b, \
            &__obfh_public_result,&__obfh_public_steps, \
            (obfh_uprt_trace *)0); \
    if (__obfh_public_status!=OBFH_UPRT_OK || \
        !__obfh_public_steps) abort(); \
    __obfh_public_result; \
})

#define VM_ADD(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_ADD,(long double)(a),(long double)(b),0u)
#define VM_SUB(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_SUB,(long double)(a),(long double)(b),0u)
#define VM_MUL(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_MUL,(long double)(a),(long double)(b),0u)
#define VM_DIV(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_DIV,(long double)(a),(long double)(b),0u)
#define VM_MOD(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_MOD,(long double)(a),(long double)(b),0u)
#define VM_EQU(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_EQ,(long double)(a),(long double)(b),0u)
#define VM_NEQ(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_NE,(long double)(a),(long double)(b),0u)
#define VM_LSS(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_LT,(long double)(a),(long double)(b),0u)
#define VM_GTR(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_GT,(long double)(a),(long double)(b),0u)
#define VM_LEQ(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_LE,(long double)(a),(long double)(b),0u)
#define VM_GEQ(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_GE,(long double)(a),(long double)(b),0u)

#define VM_AND(a,b) (unsigned int)OBFH_ARM64_VM_EXEC(OBFH_PUB_BAND,(long double)(unsigned int)(a),(long double)(unsigned int)(b),0u)
#define VM_OR(a,b) (unsigned int)OBFH_ARM64_VM_EXEC(OBFH_PUB_BOR,(long double)(unsigned int)(a),(long double)(unsigned int)(b),0u)
#define VM_XOR(a,b) (unsigned int)OBFH_ARM64_VM_EXEC(OBFH_PUB_BXOR,(long double)(unsigned int)(a),(long double)(unsigned int)(b),0u)
#define VM_NOT(a) (unsigned int)OBFH_ARM64_VM_EXEC(OBFH_PUB_BNOT,(long double)(unsigned int)(a),(long double)0,0u)
#define VM_SHL(a,b) (unsigned int)OBFH_ARM64_VM_EXEC(OBFH_PUB_BSHL,(long double)(unsigned int)(a),(long double)(unsigned int)(b),0u)
#define VM_SHR(a,b) (unsigned int)OBFH_ARM64_VM_EXEC(OBFH_PUB_BSHR,(long double)(unsigned int)(a),(long double)(unsigned int)(b),0u)

#define VM_ADD_DBL(a,b) OBFH_ARM64_VM_EXEC(OBFH_PUB_ADD,(double)(a),(double)(b),1u)
#define VM_SUB_DBL(a,b) OBFH_ARM64_VM_EXEC(OBFH_PUB_SUB,(double)(a),(double)(b),1u)
#define VM_MUL_DBL(a,b) OBFH_ARM64_VM_EXEC(OBFH_PUB_MUL,(double)(a),(double)(b),1u)
#define VM_DIV_DBL(a,b) OBFH_ARM64_VM_EXEC(OBFH_PUB_DIV,(double)(a),(double)(b),1u)
#define VM_LSS_DBL(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_LT,(double)(a),(double)(b),1u)
#define VM_GTR_DBL(a,b) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_GT,(double)(a),(double)(b),1u)
#define VM_OBF_INT(a) (long)OBFH_ARM64_VM_EXEC(OBFH_PUB_ID,(long double)(a),(long double)0,0u)
#define VM_OBF_DBL(a) OBFH_ARM64_VM_EXEC(OBFH_PUB_ID,(long double)(a),(long double)0,1u)

#define VM_IF(c) if ((int)OBFH_ARM64_VM_EXEC(OBFH_PUB_TRUTH,(long double)!!(c),(long double)0,1u))
#define VM_ELSE_IF(c) else if ((int)OBFH_ARM64_VM_EXEC(OBFH_PUB_TRUTH,(long double)!!(c),(long double)0,1u))
/* obfh_condition_true() in upstream returns true; its protected-call and
 * anti-debug side effects remain outside the qualified VM sub-contract. */
#define VM_ELSE else if ((int)OBFH_ARM64_VM_EXEC(OBFH_PUB_TRUTH,(long double)1,(long double)0,1u))
#endif
