/* SPDX-License-Identifier: MIT
 * Automatically derived source-site keys and source-formatted immutable VM.
 * Reproduces ORIGINAL 3 __COUNTER__ draws and original JUNK_WORD/MIX_A.
 * Does not claim full automatic public VM_* macro expansion.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "obfh_arm64_upstream_program_reference.h"
#include "obfh_arm64_upstream_auto_site.h"

enum { counter_before=__COUNTER__ };
OBFH_AC_DECLARE_META(auto_add,UOP_ADD);
OBFH_AC_DECLARE_META(auto_xor,UOP_BXOR);
OBFH_AC_DECLARE_META(auto_cmp,UOP_EQ);
OBFH_AC_DECLARE_META(auto_truth,UOP_TRUTH);
enum { counter_after=__COUNTER__ };
/* The first failing TinyCC seed produced exactly this high-bit key:
 * the source VM owns uint32_t key; enum sign-extension before %3 must
 * never alter the computed register permutation. */
OBFH_SC_META(highbit_contract,OBFH_SEED,UINT32_C(0xe5dd3e4a),
             UOP_ADD,5u,11u);
_Static_assert(highbit_contract_va==2u &&
               highbit_contract_vb_offset==2u &&
               highbit_contract_vb==0u,
               "High-bit VM key must use unsigned register permutation");
_Static_assert(counter_after-counter_before==17u,
               "Unexpected source-site counter consumption");
_Static_assert(auto_add_draw2==auto_add_site+3u,
               "Original source key consumes three entropy draws");
_Static_assert(sizeof(OBFH_AC_PROGRAM(auto_add))==
               auto_add_length*sizeof(uint32_t),
               "Auto-site physical array length must match original template");
_Static_assert(auto_add_length<=15u && auto_truth_length<=15u,
               "Source-selected VM site length must be bounded");
typedef char obfh_auto_typecheck[
    __builtin_types_compatible_p(
      __typeof__(OBFH_SC_CALL(auto_add,1,2,0u,3u,
                  (long double*)0,(uint32_t*)0,(obfh_uprt_trace*)0)),
      enum obfh_uprt_status) ? 1 : -1];

/* All EIGHT original template types are compiled through the compact
 * selection, not merely the four automatically keyed site samples.
 * Distinct layouts and high-bit keys catch signed enum shift regressions.
 * Fixed profiles consume zero source counters, preserving auto-site order.
 */
#define OBFH_FIXED_KEY(V,L) \
    (((uint32_t)(L)<<29u) | ((uint32_t)(V)<<24u) | \
     ((UINT32_C(0x00abc123) ^ ((uint32_t)(V)*UINT32_C(0x11321))) & \
        UINT32_C(0x00ffffff)))
OBFH_SC_META(fixed_compact_0,OBFH_SEED,OBFH_FIXED_KEY(0u,0u),UOP_ADD,0u,4u);
_Static_assert(sizeof(OBFH_AC_PROGRAM(fixed_compact_0))==4u*sizeof(uint32_t),
               "Source-selected variant 0 physical length differs");
OBFH_SC_META(fixed_compact_1,OBFH_SEED,OBFH_FIXED_KEY(1u,1u),UOP_SUB,1u,6u);
_Static_assert(sizeof(OBFH_AC_PROGRAM(fixed_compact_1))==6u*sizeof(uint32_t),
               "Source-selected variant 1 physical length differs");
OBFH_SC_META(fixed_compact_2,OBFH_SEED,OBFH_FIXED_KEY(2u,2u),UOP_MUL,2u,11u);
_Static_assert(sizeof(OBFH_AC_PROGRAM(fixed_compact_2))==11u*sizeof(uint32_t),
               "Source-selected variant 2 physical length differs");
OBFH_SC_META(fixed_compact_3,OBFH_SEED,OBFH_FIXED_KEY(3u,3u),UOP_EQ,3u,10u);
_Static_assert(sizeof(OBFH_AC_PROGRAM(fixed_compact_3))==10u*sizeof(uint32_t),
               "Source-selected variant 3 physical length differs");
OBFH_SC_META(fixed_compact_4,OBFH_SEED,OBFH_FIXED_KEY(4u,0u),UOP_TRUTH,4u,12u);
_Static_assert(sizeof(OBFH_AC_PROGRAM(fixed_compact_4))==12u*sizeof(uint32_t),
               "Source-selected variant 4 physical length differs");
OBFH_SC_META(fixed_compact_5,OBFH_SEED,OBFH_FIXED_KEY(5u,1u),UOP_BAND,5u,11u);
_Static_assert(sizeof(OBFH_AC_PROGRAM(fixed_compact_5))==11u*sizeof(uint32_t),
               "Source-selected variant 5 physical length differs");
OBFH_SC_META(fixed_compact_6,OBFH_SEED,OBFH_FIXED_KEY(6u,2u),UOP_BXOR,6u,8u);
_Static_assert(sizeof(OBFH_AC_PROGRAM(fixed_compact_6))==8u*sizeof(uint32_t),
               "Source-selected variant 6 physical length differs");
OBFH_SC_META(fixed_compact_7,OBFH_SEED,OBFH_FIXED_KEY(7u,3u),UOP_BSHL,7u,15u);
_Static_assert(sizeof(OBFH_AC_PROGRAM(fixed_compact_7))==15u*sizeof(uint32_t),
               "Source-selected variant 7 physical length differs");

static uint32_t independently_derived_key(uint32_t site,uint32_t line,
                                          uint32_t seed) {
    uint32_t junk=0u, draw[3], input;
    unsigned int i;
    for(i=0u;i<3u;++i)
        draw[i]=(((site+1u+i+line+seed)*UINT32_C(2654435761))&
                   UINT32_C(65535));
    junk=(draw[0]*UINT32_C(2246822519)) ^
         (draw[1]<<16u) ^
         (draw[2]*UINT32_C(3266489917));
    input=junk^site^seed;
    return (input^(input>>16u))*UINT32_C(2246822507);
}
static int compare_site(const obfh_sc_descriptor *s,
                        unsigned int id,unsigned int site,unsigned int line,
                        size_t storage_bytes) {
    upstream_test_program dynamic;
    unsigned int i,k,slot;
    uint32_t key=independently_derived_key(site,line,s->seed);
    if (s->key!=key) {
        fprintf(stderr,"UPAUTO_KEY_MISMATCH site=%u line=%u counter=%u actual=%08x expected=%08x\n",
                id,line,site,s->key,key);
        return 10;
    }
    if (s->variant!=((s->key>>24u)&7u) ||
        !build_program(&dynamic,s->seed,s->key,
                       s->variant,s->operation)) {
        fprintf(stderr,"UPAUTO_VARIANT_MISMATCH site=%u variant=%u key=%08x\n",
                id,s->variant,s->key);
        return 11;
    }
    if (s->length!=dynamic.length) {
        fprintf(stderr,"UPAUTO_LENGTH_MISMATCH site=%u actual=%u reference=%u\n",
                id,(unsigned int)s->length,(unsigned int)dynamic.length);
        return 12;
    }
    for (i=0u;i<s->length;++i) {
        if (s->words[i]!=dynamic.words[i]) {
            fprintf(stderr,"UPAUTO_BYTE_MISMATCH site=%u variant=%u line=%u pc=%u actual=%08x expected=%08x key=%08x\n",
                    id,s->variant,line,i,s->words[i],dynamic.words[i],s->key);
            return 13;
        }
    }
    if (storage_bytes!=s->length*sizeof(uint32_t)) {
        fprintf(stderr,"UPAUTO_STORAGE_MISMATCH site=%u bytes=%u required=%u\n",
            id,(unsigned int)storage_bytes,
            (unsigned int)(s->length*sizeof(uint32_t)));
        return 14;
    }
    for(k=0u;k<8u;++k) {
        const long double a=k==0u ? 0.0L : (long double)(k+1u);
        const long double b=k==1u ? 0.0L : 2.0L;
        unsigned int floating=(id+k)&1u;
        long double actual=-777.0L;
        long double expected=reference(s->operation,a,b,floating);
        uint32_t steps=UINT32_C(0xdecafbad);
        obfh_uprt_trace trace;
        enum obfh_uprt_status rc;
        memset(&trace,0xa5,sizeof trace);
        rc=obfh_sc_call(s,a,b,floating,UINT32_C(0x10101)+k,
                        &actual,&steps,&trace);
        if(rc!=OBFH_UPRT_OK || !equal_number(actual,expected) ||
           steps!=trace.count ||
           !expected_route(&dynamic,a,expected,&trace)) {
            fprintf(stderr,"UPAUTO_EXEC_MISMATCH site=%u test=%u rc=%u\n",
                    id,k,(unsigned int)rc);
            return 12;
        }
    }
    for(slot=0u;slot<s->length;++slot)
        if(s->words[slot]!=dynamic.words[slot]) return 13;
    return 0;
}
static int compact_variants_contract(void) {
    const uint32_t *const fixed_compact_0_words=OBFH_AC_PROGRAM(fixed_compact_0);
    const uint32_t *const fixed_compact_1_words=OBFH_AC_PROGRAM(fixed_compact_1);
    const uint32_t *const fixed_compact_2_words=OBFH_AC_PROGRAM(fixed_compact_2);
    const uint32_t *const fixed_compact_3_words=OBFH_AC_PROGRAM(fixed_compact_3);
    const uint32_t *const fixed_compact_4_words=OBFH_AC_PROGRAM(fixed_compact_4);
    const uint32_t *const fixed_compact_5_words=OBFH_AC_PROGRAM(fixed_compact_5);
    const uint32_t *const fixed_compact_6_words=OBFH_AC_PROGRAM(fixed_compact_6);
    const uint32_t *const fixed_compact_7_words=OBFH_AC_PROGRAM(fixed_compact_7);
    const obfh_sc_descriptor compact_profiles[8] = {
        OBFH_SC_DESCRIPTOR(fixed_compact_0),
        OBFH_SC_DESCRIPTOR(fixed_compact_1),
        OBFH_SC_DESCRIPTOR(fixed_compact_2),
        OBFH_SC_DESCRIPTOR(fixed_compact_3),
        OBFH_SC_DESCRIPTOR(fixed_compact_4),
        OBFH_SC_DESCRIPTOR(fixed_compact_5),
        OBFH_SC_DESCRIPTOR(fixed_compact_6),
        OBFH_SC_DESCRIPTOR(fixed_compact_7)
    };
    const size_t compact_storage_bytes[8] = {
        sizeof(OBFH_AC_PROGRAM(fixed_compact_0)),
        sizeof(OBFH_AC_PROGRAM(fixed_compact_1)),
        sizeof(OBFH_AC_PROGRAM(fixed_compact_2)),
        sizeof(OBFH_AC_PROGRAM(fixed_compact_3)),
        sizeof(OBFH_AC_PROGRAM(fixed_compact_4)),
        sizeof(OBFH_AC_PROGRAM(fixed_compact_5)),
        sizeof(OBFH_AC_PROGRAM(fixed_compact_6)),
        sizeof(OBFH_AC_PROGRAM(fixed_compact_7))
    };


    unsigned int i;
    for(i=0u;i<8u;++i) {
        const obfh_sc_descriptor *p=&compact_profiles[i];
        upstream_test_program reference_program;
        if(p->variant!=i || compact_storage_bytes[i]!=p->length*sizeof(uint32_t) ||
           !build_program(&reference_program,p->seed,p->key,i,p->operation) ||
           reference_program.length!=p->length ||
           memcmp(p->words,reference_program.words,p->length*sizeof(uint32_t))!=0) {
            fprintf(stderr,"UPCOMPACT_VARIANT_FAIL variant=%u size=%u expected=%u\n",
                i,(unsigned int)compact_storage_bytes[i],
                (unsigned int)(p->length*sizeof(uint32_t)));
            return 32;
        }
    }
    return 0;
}

static int source_key_matrix(void) {
    const uint32_t *const auto_add_words=OBFH_AC_PROGRAM(auto_add);
    const uint32_t *const auto_xor_words=OBFH_AC_PROGRAM(auto_xor);
    const uint32_t *const auto_cmp_words=OBFH_AC_PROGRAM(auto_cmp);
    const uint32_t *const auto_truth_words=OBFH_AC_PROGRAM(auto_truth);
    const obfh_sc_descriptor sites[] = {
        OBFH_SC_DESCRIPTOR(auto_add),
        OBFH_SC_DESCRIPTOR(auto_xor),
        OBFH_SC_DESCRIPTOR(auto_cmp),
        OBFH_SC_DESCRIPTOR(auto_truth)
    };
    /* Unlike the previous padded 15-word object, sizeof the selected compound
     * array yields its ACTUAL physical storage bytes per distinct source site.
     * The reference builder supplies an independent expected length. */
    const size_t site_storage_bytes[] = {
        sizeof(OBFH_AC_PROGRAM(auto_add)),
        sizeof(OBFH_AC_PROGRAM(auto_xor)),
        sizeof(OBFH_AC_PROGRAM(auto_cmp)),
        sizeof(OBFH_AC_PROGRAM(auto_truth))
    };


    const unsigned int counter[4]={
        auto_add_site,auto_xor_site,auto_cmp_site,auto_truth_site};
    const unsigned int line[4]={
        auto_add_line,auto_xor_line,auto_cmp_line,auto_truth_line};
    unsigned int i;
    int result;
    for(i=0u;i<4u;++i) {
        if(i && counter[i]!=counter[i-1]+4u) return 20;
        result=compare_site(&sites[i],i,counter[i],line[i],site_storage_bytes[i]);
        if(result) return result;
    }
    if(sites[0].key==sites[1].key ||
       sites[1].key==sites[2].key ||
       sites[2].key==sites[3].key) return 21;
    return 0;
}
static int argument_single_evaluation(void) {
    const uint32_t *const auto_add_words=OBFH_AC_PROGRAM(auto_add);
    unsigned int a=0u,b=0u,f=0u,n=0u;
    long double result=-777.0L;
    uint32_t steps=0u;
    obfh_uprt_trace trace;
    enum obfh_uprt_status rc=OBFH_SC_CALL(auto_add,
          (++a,5.0L),(++b,3.0L),(++f,0u),(++n,11u),
          &result,&steps,&trace);
    if(rc!=OBFH_UPRT_OK || result!=8.0L ||
       a!=1u || b!=1u || f!=1u || n!=1u ||
       steps!=trace.count) return 30;
    return 0;
}
int main(void) {
    int rc=source_key_matrix();
    if(rc) return rc;
    rc=compact_variants_contract();
    if(rc) return rc;
    rc=argument_single_evaluation();
    if(rc) return rc;
    printf("UPSTREAM_AUTO_SITE_PASS: %u sites, 3 draws per site\n",4u);
    return 0;
}
