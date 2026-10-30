/* SPDX-License-Identifier: MIT
 * Verify original two independent compile-time operand RND counters:
 * RND(1,2147483647u), not a runtime RNG or a site-key-derived shortcut.
 * Both operands are encoded independently and dispatched through the
 * same accepted original-format AArch64 VM. No extra interpreter.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "obfh_arm64_upstream_program_reference.h"
#include "obfh_arm64_upstream_auto_site.h"

#define NONCE_SITE(NAME,OP) \
    OBFH_AS_DECLARE(NAME,OP); \
    OBFH_AS_DECLARE_NONCES(NAME)
enum { nonce_before=__COUNTER__ };
NONCE_SITE(nonce_add,UOP_ADD);
NONCE_SITE(nonce_xor,UOP_BXOR);
NONCE_SITE(nonce_eq,UOP_EQ);
NONCE_SITE(nonce_truth,UOP_TRUTH);
enum { nonce_after=__COUNTER__ };

_Static_assert(nonce_after-nonce_before==25u,
               "Four VM sites must consume four keys plus eight RND nonces");
_Static_assert(nonce_add_operand_counter_a==nonce_add_site+4u,
               "First RND must follow original four source key counters");
_Static_assert(nonce_add_operand_counter_b==nonce_add_site+5u,
               "Second operand RND must consume next independent counter");
_Static_assert(nonce_xor_site==nonce_add_site+6u,
               "No surplus counters between adjacent operand VM calls");

typedef struct {
    obfh_sc_descriptor site;
    uint32_t id,line,nonce_count_a,nonce_count_b,nonce_a,nonce_b;
} nonce_fixture;
static const nonce_fixture fixtures[] = {
    {OBFH_SC_DESCRIPTOR(nonce_add),nonce_add_site,nonce_add_line,
      nonce_add_operand_counter_a,nonce_add_operand_counter_b,
      nonce_add_nonce_a,nonce_add_nonce_b},
    {OBFH_SC_DESCRIPTOR(nonce_xor),nonce_xor_site,nonce_xor_line,
      nonce_xor_operand_counter_a,nonce_xor_operand_counter_b,
      nonce_xor_nonce_a,nonce_xor_nonce_b},
    {OBFH_SC_DESCRIPTOR(nonce_eq),nonce_eq_site,nonce_eq_line,
      nonce_eq_operand_counter_a,nonce_eq_operand_counter_b,
      nonce_eq_nonce_a,nonce_eq_nonce_b},
    {OBFH_SC_DESCRIPTOR(nonce_truth),nonce_truth_site,nonce_truth_line,
      nonce_truth_operand_counter_a,nonce_truth_operand_counter_b,
      nonce_truth_nonce_a,nonce_truth_nonce_b}
};

/* Independently transcribed upstream RND(max 2147483647), with explicit
 * unsigned 32-bit wrap BEFORE the 2147483647 modulo. */
static uint32_t source_rnd31(uint32_t counter,uint32_t line,uint32_t seed) {
    uint32_t value=(counter+line+seed)*UINT32_C(2654435761);
    return UINT32_C(1)+(value % UINT32_C(2147483647));
}

static int pair_contract(void) {
    unsigned int i,round;
    for(i=0u;i<sizeof fixtures/sizeof fixtures[0];++i) {
        const nonce_fixture *f=&fixtures[i];
        const obfh_sc_descriptor *site=&f->site;
        upstream_test_program expected_program;
        long double left=i==0u?5.0L:i==1u?7.0L:i==2u?3.0L:1.0L;
        long double right=i==0u?3.0L:i==1u?2.0L:i==2u?3.0L:0.0L;
        obfh_arm64_upstream_value a,b,copy;
        uint32_t generated_a=source_rnd31(f->nonce_count_a,f->line,site->seed);
        uint32_t generated_b=source_rnd31(f->nonce_count_b,f->line,site->seed);
        if(f->nonce_count_a!=f->id+4u ||
           f->nonce_count_b!=f->id+5u ||
           generated_a!=f->nonce_a ||
           generated_b!=f->nonce_b ||
           generated_a==generated_b) {
            fprintf(stderr,"UPNONCE_COUNTER_MISMATCH site=%u counter=%u line=%u actual=%u/%u expected=%u/%u\n",
                  i,f->id,f->line,f->nonce_a,f->nonce_b,generated_a,generated_b);
            return 10;
        }
        if(!build_program(&expected_program,site->seed,site->key,
                          site->variant,site->operation) ||
           site->length!=expected_program.length ||
           memcmp(site->words,expected_program.words,
                  site->length*sizeof(uint32_t))!=0)
            return 11;

        if(obfh_arm64_upvalue_encode((const unsigned char*)&left,
             sizeof left,site->seed,17,(f->nonce_a<<1u)|1u,&a)!=OBFH_UPVALUE_OK ||
           obfh_arm64_upvalue_encode((const unsigned char*)&right,
             sizeof right,site->seed,41,(f->nonce_b<<1u)|1u,&b)!=OBFH_UPVALUE_OK)
            return 12;
        if(a.nonce!=(f->nonce_a^site->seed) ||
           b.nonce!=(f->nonce_b^site->seed) ||
           a.floating!=1u || b.floating!=1u) return 13;
        /* Same source operand with two independent counters must produce
         * distinct encoded packets even when the salt is held constant. */
        if(obfh_arm64_upvalue_encode((const unsigned char*)&left,
             sizeof left,site->seed,17,(f->nonce_b<<1u)|1u,&copy)!=OBFH_UPVALUE_OK ||
           a.nonce==copy.nonce ||
           memcmp(&a,&copy,sizeof a)==0) return 14;

        for(round=0u;round<4u;++round) {
            long double result=-999.0L;
            long double expected=reference(site->operation,left,right,1u);
            obfh_uprt_trace trace;
            uint32_t steps=UINT32_C(0xdecafbad);
            enum obfh_uprt_status status=obfh_sc_call_nonces(
                  site,left,right,1u,
                  f->nonce_a,f->nonce_b,&result,&steps,&trace);
            if(status!=OBFH_UPRT_OK ||
               !equal_number(result,expected) ||
               steps!=trace.count ||
               !expected_route(&expected_program,left,expected,&trace)) {
                fprintf(stderr,"UPNONCE_VM_FAIL site=%u round=%u status=%u\n",
                        i,round,(unsigned int)status);
                return 15;
            }
        }
    }
    return 0;
}
static int negative_contract(void) {
    const obfh_sc_descriptor *s=&fixtures[0].site;
    long double output=-999.0L;
    uint32_t steps=UINT32_C(0xdecafbad);
    obfh_uprt_trace trace,backup;
    enum obfh_uprt_status result;
    memset(&trace,0x5c,sizeof trace); backup=trace;
#define WRONG(A,B) do { \
    result=obfh_sc_call_nonces(s,5.0L,3.0L,0u, \
                  (A),(B),&output,&steps,&trace); \
    if(result!=OBFH_UPRT_INVALID_ARGS || output!=-999.0L || \
       steps!=UINT32_C(0xdecafbad) || \
       memcmp(&trace,&backup,sizeof trace)!=0) return 21; \
} while(0)
    WRONG(0u,fixtures[0].nonce_b);
    WRONG(fixtures[0].nonce_a,0u);
    WRONG(UINT32_C(0x80000000),fixtures[0].nonce_b);
    WRONG(fixtures[0].nonce_a,UINT32_MAX);
#undef WRONG
    return 0;
}
int main(void) {
    int e=pair_contract();
    if(e)return e;
    e=negative_contract();
    if(e)return e;
    printf("UPSTREAM_ORIGINAL_TWO_NONCES_PASS: 4 sites\n");
    return 0;
}
