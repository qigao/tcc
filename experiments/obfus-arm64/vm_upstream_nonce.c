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
#include "obfh_arm64_upstream_vm_public.h"

#define NONCE_SITE(NAME,OP) \
    OBFH_AC_DECLARE_META(NAME,OP); \
    OBFH_AS_DECLARE_NONCES(NAME)
/* Independent fixed golden constants from upstream's uint32 wrap + modulo.
 * The previous Windows TinyCC candidate wrongly retained a 64-bit product
 * until the range modulo, producing 1146285700 instead of 768765920.
 */
_Static_assert(OBFH_AS_RND31(5u,18u,UINT32_C(0x12345678))==
               UINT32_C(768765920),
               "Original first operand 32-bit RND golden");
_Static_assert(OBFH_AS_RND31(6u,18u,UINT32_C(0x12345678))==
               UINT32_C(1275718034),
               "Original second operand 32-bit RND golden");
_Static_assert(OBFH_AS_RND31(5u,18u,UINT32_C(0x12345679))==
               UINT32_C(1275718034),
               "Original alternate-seed operand 32-bit RND golden");
_Static_assert(OBFH_AS_RND31(6u,18u,UINT32_C(0x12345679))==
               UINT32_C(1782670146),
               "Original alternate-seed B operand 32-bit RND golden");
/* Two header salt RND(16,48) calls must precede test source-site
 * counters and must use the exact unsigned wrap before modulo 33.
 * Golden values are independently calculated from upstream expressions.
 */
_Static_assert(OBFH_AS_RND_SALT(5u,18u,UINT32_C(0x12345678))==47u,
               "Original header salt A golden seed even");
_Static_assert(OBFH_AS_RND_SALT(6u,18u,UINT32_C(0x12345678))==24u,
               "Original header salt B golden seed even");
_Static_assert(OBFH_AS_RND_SALT(5u,18u,UINT32_C(0x12345679))==24u,
               "Original header salt A golden seed odd");
_Static_assert(OBFH_AS_RND_SALT(6u,18u,UINT32_C(0x12345679))==30u,
               "Original header salt B golden seed odd");

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
    uint32_t seed,key;
    unsigned int variant,operation;
    size_t length;
    uint32_t id,line,nonce_count_a,nonce_count_b,nonce_a,nonce_b;
} nonce_fixture;
/* Only numeric metadata is file-scope: the exact-size original program
 * compound literal is constructed in its EXECUTION scope below. */
#define NONCE_META(NAME) { \
    (uint32_t)OBFH_SEED,(uint32_t)NAME##_key,NAME##_variant, \
    NAME##_operation,NAME##_length,NAME##_site,NAME##_line, \
    NAME##_operand_counter_a,NAME##_operand_counter_b, \
    NAME##_nonce_a,NAME##_nonce_b }
static const nonce_fixture fixtures[] = {
    NONCE_META(nonce_add),NONCE_META(nonce_xor),
    NONCE_META(nonce_eq),NONCE_META(nonce_truth)
};

/* Independently transcribed upstream RND(max 2147483647), with explicit
 * unsigned 32-bit wrap BEFORE the 2147483647 modulo. */
static uint32_t source_rnd31(uint32_t counter,uint32_t line,uint32_t seed) {
    uint32_t value=(counter+line+seed)*UINT32_C(2654435761);
    return UINT32_C(1)+(value % UINT32_C(2147483647));
}

/* Independent C reference to source's RND(16,48). The multiplication
 * is stored in an actual uint32_t before performing range modulo.
 */
static uint32_t source_rnd_salt(uint32_t counter,uint32_t line,uint32_t seed) {
    uint32_t word=(counter+line+seed)*UINT32_C(2654435761);
    return 16u+(word%33u);
}
static int header_salt_contract(void) {
    const uint32_t salt_a=source_rnd_salt(
        OBFH_PUB_SALT_COUNTER_A,OBFH_PUB_SALT_LINE_A,(uint32_t)OBFH_SEED);
    const uint32_t salt_b=source_rnd_salt(
        OBFH_PUB_SALT_COUNTER_B,OBFH_PUB_SALT_LINE_B,(uint32_t)OBFH_SEED);
    if (OBFH_PUB_SALT_COUNTER_B!=OBFH_PUB_SALT_COUNTER_A+1u ||
        OBFH_PUB_SALT_LINE_B!=OBFH_PUB_SALT_LINE_A+1u ||
        (uint32_t)OBFH_PUB_SALT_NUM1!=salt_a ||
        (uint32_t)OBFH_PUB_SALT_NUM2!=salt_b ||
        salt_a<16u || salt_a>48u || salt_b<16u || salt_b>48u) {
        fprintf(stderr,"UPSALT_HEADER_MISMATCH count=%u/%u line=%u/%u actual=%u/%u reference=%u/%u\n",
                OBFH_PUB_SALT_COUNTER_A,OBFH_PUB_SALT_COUNTER_B,
                OBFH_PUB_SALT_LINE_A,OBFH_PUB_SALT_LINE_B,
                OBFH_PUB_SALT_NUM1,OBFH_PUB_SALT_NUM2,salt_a,salt_b);
        return 16;
    }
    /* Verify identical inputs can be transported with *different salts*
     * and source-specific nonces, with no double representation fallback.
     */
    {
        long double left=5.25L,right=3.5L,decoded=-777.0L;
        obfh_arm64_upstream_value a,b;
        unsigned char raw[sizeof(long double)];
        uint32_t na=source_rnd31(5u,18u,(uint32_t)OBFH_SEED);
        uint32_t nb=source_rnd31(6u,18u,(uint32_t)OBFH_SEED);
        if(obfh_arm64_upvalue_encode((const unsigned char*)&left,
               sizeof left,(uint32_t)OBFH_SEED,(int)salt_a,
               (na<<1u)|1u,&a)!=OBFH_UPVALUE_OK ||
           obfh_arm64_upvalue_encode((const unsigned char*)&right,
               sizeof right,(uint32_t)OBFH_SEED,(int)salt_b,
               (nb<<1u)|1u,&b)!=OBFH_UPVALUE_OK ||
           obfh_arm64_upvalue_decode(&a,(uint32_t)OBFH_SEED,(int)salt_a,
               raw,sizeof raw)!=OBFH_UPVALUE_OK ||
           memcmp(&left,raw,sizeof left)!=0) return 17;
        if(obfh_arm64_upvalue_decode(&b,(uint32_t)OBFH_SEED,(int)salt_b,
               raw,sizeof raw)!=OBFH_UPVALUE_OK) return 18;
        memcpy(&decoded,raw,sizeof decoded);
        if(decoded!=right || a.nonce==(b.nonce)) return 19;
    }
    return 0;
}

static int pair_contract(void) {
    const uint32_t *const code[4] = {
        OBFH_AC_PROGRAM(nonce_add),OBFH_AC_PROGRAM(nonce_xor),
        OBFH_AC_PROGRAM(nonce_eq),OBFH_AC_PROGRAM(nonce_truth)
    };
    const size_t sizes[4] = {
        sizeof(OBFH_AC_PROGRAM(nonce_add)),
        sizeof(OBFH_AC_PROGRAM(nonce_xor)),
        sizeof(OBFH_AC_PROGRAM(nonce_eq)),
        sizeof(OBFH_AC_PROGRAM(nonce_truth))
    };
    unsigned int i,round;
    for(i=0u;i<sizeof fixtures/sizeof fixtures[0];++i) {
        const nonce_fixture *f=&fixtures[i];
        const obfh_sc_descriptor descriptor = {
            code[i],f->length,f->seed,f->key,f->variant,f->operation
        };
        const obfh_sc_descriptor *site=&descriptor;
        if(sizes[i]!=f->length*sizeof(uint32_t)) return 23;
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
            /* Keep historical salt 17/41 and new source-header salt
             * profiles in the SAME VM differential (no second engine).
             */
            enum obfh_uprt_status status=round<2u ?
                obfh_sc_call_nonces(site,left,right,1u,
                    f->nonce_a,f->nonce_b,&result,&steps,&trace) :
                obfh_sc_call_profiled(site,left,right,1u,
                    f->nonce_a,f->nonce_b,
                    OBFH_PUB_SALT_NUM1,OBFH_PUB_SALT_NUM2,
                    &result,&steps,&trace);
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
    const uint32_t *const code=OBFH_AC_PROGRAM(nonce_add);
    const obfh_sc_descriptor descriptor = {
       code,nonce_add_length,(uint32_t)OBFH_SEED,(uint32_t)nonce_add_key,
       nonce_add_variant,nonce_add_operation
    };
    const obfh_sc_descriptor *s=&descriptor;
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
#define BAD_SALT(SA,SB) do { \
    output=-999.0L;steps=UINT32_C(0xdecafbad);trace=backup; \
    result=obfh_sc_call_profiled(s,5.0L,3.0L,0u, \
            fixtures[0].nonce_a,fixtures[0].nonce_b, \
            (SA),(SB),&output,&steps,&trace); \
    if(result!=OBFH_UPRT_INVALID_ARGS || output!=-999.0L || \
       steps!=UINT32_C(0xdecafbad) || \
       memcmp(&trace,&backup,sizeof trace)!=0) return 22; \
} while(0)
    BAD_SALT(15,OBFH_PUB_SALT_NUM2);
    BAD_SALT(OBFH_PUB_SALT_NUM1,49);
    BAD_SALT(-1,OBFH_PUB_SALT_NUM2);
    BAD_SALT(OBFH_PUB_SALT_NUM1,0);
#undef BAD_SALT
    return 0;
}
int main(void) {
    int e=header_salt_contract();
    if(e)return e;
    e=pair_contract();
    if(e)return e;
    e=negative_contract();
    if(e)return e;
    printf("UPSTREAM_ORIGINAL_NONCES_AND_HEADER_SALTS_PASS: 4 sites, salts=%u/%u\n",
           OBFH_PUB_SALT_NUM1,OBFH_PUB_SALT_NUM2);
    return 0;
}
