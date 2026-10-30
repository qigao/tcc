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
OBFH_AS_DECLARE(auto_add,UOP_ADD);
OBFH_AS_DECLARE(auto_xor,UOP_BXOR);
OBFH_AS_DECLARE(auto_cmp,UOP_EQ);
OBFH_AS_DECLARE(auto_truth,UOP_TRUTH);
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
_Static_assert(sizeof auto_add_words==15u*sizeof(uint32_t),
               "Automatic site allocation must be bounded");
_Static_assert(auto_add_length<=15u && auto_truth_length<=15u,
               "Source-selected VM site length must be bounded");
typedef char obfh_auto_typecheck[
    __builtin_types_compatible_p(
      __typeof__(OBFH_SC_CALL(auto_add,1,2,0u,3u,
                  (long double*)0,(uint32_t*)0,(obfh_uprt_trace*)0)),
      enum obfh_uprt_status) ? 1 : -1];

static const obfh_sc_descriptor sites[] = {
    OBFH_SC_DESCRIPTOR(auto_add),
    OBFH_SC_DESCRIPTOR(auto_xor),
    OBFH_SC_DESCRIPTOR(auto_cmp),
    OBFH_SC_DESCRIPTOR(auto_truth)
};

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
                        unsigned int id,unsigned int site,unsigned int line) {
    upstream_test_program dynamic;
    unsigned int i,k,slot;
    uint32_t key=independently_derived_key(site,line,s->seed);
    long double out;
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
    for(i=(unsigned int)s->length;i<15u;++i)
        if(s->words[i]!=0u) return 11;
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
static int source_key_matrix(void) {
    const unsigned int counter[4]={
        auto_add_site,auto_xor_site,auto_cmp_site,auto_truth_site};
    const unsigned int line[4]={
        auto_add_line,auto_xor_line,auto_cmp_line,auto_truth_line};
    unsigned int i;
    int result;
    for(i=0u;i<4u;++i) {
        if(i && counter[i]!=counter[i-1]+4u) return 20;
        result=compare_site(&sites[i],i,counter[i],line[i]);
        if(result) return result;
    }
    if(sites[0].key==sites[1].key ||
       sites[1].key==sites[2].key ||
       sites[2].key==sites[3].key) return 21;
    return 0;
}
static int argument_single_evaluation(void) {
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
    rc=argument_single_evaluation();
    if(rc) return rc;
    printf("UPSTREAM_AUTO_SITE_PASS: %u sites, 3 draws per site\n",4u);
    return 0;
}
