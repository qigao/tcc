/* SPDX-License-Identifier: MIT
 * Exact source-formatted static immutable instruction arrays at distinct C
 * declaration sites. Uses the SAME integrated VM; no second runtime engine.
 * All eight source template shapes are compared word-by-word against the
 * one source-shaped dynamic reference builder, then run on native AArch64.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "obfh_arm64_upstream_program_reference.h"
#include "obfh_arm64_upstream_static.h"

/* For this explicit-site phase, caller supplies the key and variant.
 * Matching is enforced by _Static_assert in each declaration. The upstream
 * __COUNTER__-derived automatic site-key facility is a separate open gate.
 */
#define SITE_KEY(V,LAYOUT,SALT) \
    (((uint32_t)(LAYOUT)<<29u)|((uint32_t)(V)<<24u)| \
     (((uint32_t)OBFH_SEED ^ (uint32_t)(SALT))&UINT32_C(0x00ffffff)))
OBFH_SC_DECLARE_0(site0,OBFH_SEED,SITE_KEY(0u,0u,0x10100u),UOP_ADD);
OBFH_SC_DECLARE_1(site1,OBFH_SEED,SITE_KEY(1u,1u,0x20220u),UOP_SUB);
OBFH_SC_DECLARE_2(site2,OBFH_SEED,SITE_KEY(2u,2u,0x30340u),UOP_MUL);
OBFH_SC_DECLARE_3(site3,OBFH_SEED,SITE_KEY(3u,3u,0x40460u),UOP_EQ);
OBFH_SC_DECLARE_4(site4,OBFH_SEED,SITE_KEY(4u,0u,0x50580u),UOP_TRUTH);
OBFH_SC_DECLARE_5(site5,OBFH_SEED,SITE_KEY(5u,1u,0x606a0u),UOP_BAND);
OBFH_SC_DECLARE_6(site6,OBFH_SEED,SITE_KEY(6u,2u,0x707c0u),UOP_BXOR);
OBFH_SC_DECLARE_7(site7,OBFH_SEED,SITE_KEY(7u,3u,0x808e0u),UOP_BSHL);
/* Same selector/variant but distinct key and layout means distinct bytes. */
OBFH_SC_DECLARE_0(site0_second,OBFH_SEED,SITE_KEY(0u,1u,0x91911u),UOP_ADD);
OBFH_SC_DECLARE_7(site7_second,OBFH_SEED,SITE_KEY(7u,0u,0xa2a22u),UOP_BSHL);

#ifdef OBFH_STATIC_VARIANT_NEGATIVE_TEST
/* Must fail *during compilation* instead of quietly selecting a variant. */
OBFH_SC_DECLARE_0(site_bad,OBFH_SEED,SITE_KEY(1u,0u,0x9911u),UOP_ADD);
#endif

_Static_assert(sizeof site0_words==4u*sizeof(uint32_t), "variant0 length");
_Static_assert(sizeof site1_words==6u*sizeof(uint32_t), "variant1 length");
_Static_assert(sizeof site2_words==11u*sizeof(uint32_t), "variant2 length");
_Static_assert(sizeof site3_words==10u*sizeof(uint32_t), "variant3 length");
_Static_assert(sizeof site4_words==12u*sizeof(uint32_t), "variant4 length");
_Static_assert(sizeof site5_words==11u*sizeof(uint32_t), "variant5 length");
_Static_assert(sizeof site6_words==8u*sizeof(uint32_t), "variant6 length");
_Static_assert(sizeof site7_words==15u*sizeof(uint32_t), "variant7 length");
/* A function-call expression, unlike GNU ({...}), is valid here. */
typedef char obfh_static_typecheck[
    __builtin_types_compatible_p(
       __typeof__(OBFH_SC_CALL(site0,1,2,0u,3u,
                    (long double *)0,(uint32_t *)0,(obfh_uprt_trace *)0)),
       enum obfh_uprt_status) ? 1 : -1];

static const obfh_sc_descriptor sites[] = {
    OBFH_SC_DESCRIPTOR(site0), OBFH_SC_DESCRIPTOR(site1),
    OBFH_SC_DESCRIPTOR(site2), OBFH_SC_DESCRIPTOR(site3),
    OBFH_SC_DESCRIPTOR(site4), OBFH_SC_DESCRIPTOR(site5),
    OBFH_SC_DESCRIPTOR(site6), OBFH_SC_DESCRIPTOR(site7),
    OBFH_SC_DESCRIPTOR(site0_second), OBFH_SC_DESCRIPTOR(site7_second)
};

static uint64_t hash_words(const uint32_t *words,size_t count) {
    uint64_t h=UINT64_C(0xcbf29ce484222325);
    size_t i;
    unsigned int j;
    for (i=0u;i<count;++i)for (j=0u;j<4u;++j) {
        h ^= (words[i]>>(8u*j))&255u;
        h *= UINT64_C(0x100000001b3);
    }
    return h;
}
static int static_equivalence(void) {
    unsigned int i,k;
    if (hash_words(site0_words,site0_length)==
        hash_words(site0_second_words,site0_second_length)) return 10;
    if (hash_words(site7_words,site7_length)==
        hash_words(site7_second_words,site7_second_length)) return 11;
    for (i=0u;i<sizeof sites/sizeof sites[0];++i) {
        const obfh_sc_descriptor *s=&sites[i];
        upstream_test_program reference_program;
        const uint64_t before=hash_words(s->words,s->length);
        if (!build_program(&reference_program,s->seed,s->key,
                            s->variant,s->operation) ||
            reference_program.length!=s->length ||
            memcmp(s->words,reference_program.words,
                   s->length*sizeof(uint32_t))!=0) {
            fprintf(stderr,"UPSTATIC_BYTE_MISMATCH site=%u variant=%u opcode=%u\n",
                    i,s->variant,s->operation);
            return 20;
        }
        for (k=0u;k<6u;++k) {
            const long double a=k==0u ? 0.0L : (long double)(k*3u);
            const long double b=k==1u ? 0.0L : 3.0L;
            unsigned int floating=(i+k)&1u;
            long double actual=-777.0L,expected=reference(s->operation,a,b,floating);
            uint32_t steps=UINT32_C(0xdecafbad);
            obfh_uprt_trace route;
            enum obfh_uprt_status status;
            memset(&route,0xa5,sizeof route);
            status=obfh_sc_call(s,a,b,floating,i*8192u+k+1u,
                                &actual,&steps,&route);
            if (status!=OBFH_UPRT_OK ||
                !equal_number(actual,expected) ||
                steps!=route.count ||
                !expected_route(&reference_program,a,expected,&route)) {
                fprintf(stderr,"UPSTATIC_EXEC_FAIL site=%u sample=%u status=%u\n",
                        i,k,(unsigned int)status);
                return 21;
            }
        }
        if (before!=hash_words(s->words,s->length)) return 22;
    }
    return 0;
}
static int single_eval(void) {
    unsigned int a_calls=0u,b_calls=0u,f_calls=0u,n_calls=0u;
    long double result=-777.0L;
    uint32_t steps=0u;
    obfh_uprt_trace trace;
    enum obfh_uprt_status status;
    status=OBFH_SC_CALL(site0,(++a_calls,5.0L),(++b_calls,2.0L),
          (++f_calls,0u),(++n_calls,123u),&result,&steps,&trace);
    if (status!=OBFH_UPRT_OK || result!=7.0L || steps!=4u ||
        a_calls!=1u || b_calls!=1u || f_calls!=1u || n_calls!=1u)
        return 30;
    return 0;
}
static int fail_closed(void) {
    long double result=-777.0L;
    uint32_t steps=UINT32_C(0xdecafbad);
    obfh_uprt_trace trace,original;
    enum obfh_uprt_status got;
    memset(&trace,0x5c,sizeof trace);
    original=trace;
    got=obfh_sc_call(&sites[0],5.0L,2.0L,2u,1u,
                     &result,&steps,&trace);
    if (got!=OBFH_UPRT_INVALID_ARGS || result!=-777.0L ||
        steps!=UINT32_C(0xdecafbad) ||
        memcmp(&trace,&original,sizeof trace)!=0)
        return 40;
    got=obfh_sc_call((const obfh_sc_descriptor*)0,5.0L,2.0L,0u,1u,
                     &result,&steps,&trace);
    if (got!=OBFH_UPRT_INVALID_ARGS || result!=-777.0L ||
        steps!=UINT32_C(0xdecafbad)) return 41;
    return 0;
}
int main(void) {
    int rc=static_equivalence();
    if (rc) return rc;
    rc=single_eval();
    if (rc) return rc;
    return fail_closed();
}
