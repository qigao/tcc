/* SPDX-License-Identifier: MIT
 * Original spelling VM_* API smoke through source-keyed static packed VM code.
 * Every call goes to obfh_arm64_upstream_execute; NO plain-C operation fallback.
 */
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include "obfh_arm64_upstream_vm_public.h"

static int arithmetic_contract(void) {
    unsigned int a=0u,b=0u;
    if (VM_ADD(5,7)!=12L || VM_SUB(5,7)!=-2L ||
        VM_MUL(-3,7)!=-21L) return 10;
    if (VM_DIV(7,2)!=3L || VM_MOD(-7,3)!=-1L) return 11;
    if (VM_EQU(5,5)!=1L || VM_NEQ(5,5)!=0L ||
        VM_LSS(2,5)!=1L || VM_GTR(9,5)!=1L ||
        VM_LEQ(5,5)!=1L || VM_GEQ(5,5)!=1L) return 12;
    if (VM_ADD(++a,++b)!=2L || a!=1u || b!=1u) return 13;
    if (VM_OBF_INT(-7)!=-7L) return 14;
    return 0;
}
static int bitwise_contract(void) {
    unsigned int a=0u,b=0u;
    if (VM_AND(0xf0f0u,0x0ff0u)!=0x00f0u ||
        VM_OR(0xf000u,0x000fu)!=0xf00fu ||
        VM_XOR(0xaaaau,0x5555u)!=0xffffu) return 20;
    if (VM_NOT(0u)!=UINT_MAX || VM_SHL(1u,31u)!=0x80000000u ||
        VM_SHR(UINT_MAX,31u)!=1u) return 21;
    if (VM_XOR(++a,++b)!=0u || a!=1u || b!=1u) return 22;
    return 0;
}
static int floating_contract(void) {
    if (VM_ADD_DBL(5.5,3.25)!=8.75L ||
        VM_SUB_DBL(5.5,3.25)!=2.25L ||
        VM_MUL_DBL(2.0,4.0)!=8.0L ||
        VM_DIV_DBL(6.0,4.0)!=1.5L) return 30;
    if (VM_LSS_DBL(1.5,2.5)!=1L ||
        VM_GTR_DBL(2.5,1.5)!=1L ||
        VM_OBF_DBL(-7.25L)!=-7.25L) return 31;
    return 0;
}
static int branch_contract(void) {
    unsigned int count=0u,selected=0u;
    VM_IF(++count==0u) { selected=1u; }
    VM_ELSE_IF(++count==2u) { selected=2u; }
    VM_ELSE { selected=3u; }
    if (selected!=2u || count!=2u) return 40;

    count=0u; selected=0u;
    VM_IF(0 && ++count) { selected=1u; }
    VM_ELSE_IF(1 || ++count) { selected=2u; }
    VM_ELSE { selected=3u; }
    if (selected!=2u || count!=0u) return 41;

    selected=0u;
    VM_IF(VM_EQU(VM_ADD(2,2),4)) { selected=9u; }
    VM_ELSE { selected=10u; }
    if (selected!=9u) return 42;
    return 0;
}
int main(void) {
    int rc=arithmetic_contract();
    if(rc) { fprintf(stderr,"VM_PUBLIC_ARITH_FAIL %d\n",rc);return rc; }
    rc=bitwise_contract();
    if(rc) { fprintf(stderr,"VM_PUBLIC_BITS_FAIL %d\n",rc);return rc; }
    rc=floating_contract();
    if(rc) { fprintf(stderr,"VM_PUBLIC_FLOAT_FAIL %d\n",rc);return rc; }
    rc=branch_contract();
    if(rc) { fprintf(stderr,"VM_PUBLIC_BRANCH_FAIL %d\n",rc);return rc; }
    puts("ORIGINAL_VM_PUBLIC_MACROS_PASS");
    return 0;
}
