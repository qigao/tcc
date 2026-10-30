/* SPDX-License-Identifier: MIT
 * Original obfus.h region-09 OBFH_VM_VALUE object-byte differential.
 * Independent source-shaped oracle + fixed golden digest for both known
 * native ABI widths (8-byte and 16-byte long double).
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "obfh_arm64_upstream_value.h"

/* This is a separate reference encoding path, not the header's step helper.
 * A fixed external golden digest prevents two mutually wrong implementations
 * from passing solely because roundtrips agree.
 */
static void oracle_encode(const unsigned char *src, uint32_t seed, int salt,
                          uint32_t floating_nonce,
                          obfh_arm64_upstream_value *out) {
    uint32_t state;
    unsigned int first=((seed>>22u)&1u) ? 5u : 13u;
    unsigned int last=((seed>>22u)&1u) ? 13u : 5u;
    size_t n=sizeof(long double), offset, i;
    out->nonce=(floating_nonce>>1u)^seed;
    out->floating=(unsigned char)(floating_nonce&1u);
    state=out->nonce^((uint32_t)salt*UINT32_C(2246822519))^UINT32_C(0x9e3779b9);
    offset=out->nonce%n;
    for (i=0;i<n;++i) {
        unsigned int rotate, x;
        unsigned char a,b;
        if ((seed>>21u)&1u) {
            state^=state>>first;
            state^=state<<17u;
            state^=state>>last;
        } else {
            state^=state<<first;
            state^=state>>17u;
            state^=state<<last;
        }
        rotate=(state>>24u)&7u;
        x=((unsigned int)src[i]<<rotate)|
          ((unsigned int)src[i]>>(8u-rotate));
        a=(unsigned char)(state>>8u);
        b=(unsigned char)(state^(state>>16u));
        if ((seed>>10u)&1u)
            out->bytes[(i+offset)%n]=(unsigned char)(((x&255u)^b)+a);
        else
            out->bytes[(i+offset)%n]=(unsigned char)(((x+a)&255u)^b);
    }
}

static uint64_t add_digest(uint64_t h, const obfh_arm64_upstream_value *p) {
    size_t i;
    unsigned int k;
    for (i=0;i<sizeof(long double);++i) {
        h^=p->bytes[i];
        h*=UINT64_C(0x100000001b3);
    }
    h^=p->floating;
    h*=UINT64_C(0x100000001b3);
    for (k=0u;k<4u;++k) {
        h^=(p->nonce>>(8u*k))&255u;
        h*=UINT64_C(0x100000001b3);
    }
    return h;
}

static int golden_cross_modes(void) {
    unsigned char raw[sizeof(long double)], restored[sizeof(long double)];
    uint64_t digest=UINT64_C(0xcbf29ce484222325);
    unsigned int mode,v,f,i;
    if (sizeof(long double)!=8u && sizeof(long double)!=16u) return 11;
    for (mode=0u;mode<8u;++mode) {
        uint32_t seed=UINT32_C(0x20100031)|
            ((mode&1u)<<10u)|(((mode>>1u)&1u)<<21u)|
            (((mode>>2u)&1u)<<22u);
        for (v=0u;v<8u;++v) {
            int salt=16+(int)((mode*7u+v*5u)%33u);
            uint32_t nonce31=(((v+1u)*UINT32_C(0x1020305))+
                              mode*UINT32_C(0x13579))&UINT32_C(0x7fffffff);
            for (i=0u;i<sizeof raw;++i)
                raw[i]=(unsigned char)(i*73u+v*19u+(i^v)*11u);
            for (f=0u;f<2u;++f) {
                obfh_arm64_upstream_value pkt,expected;
                uint32_t floating_nonce=(nonce31<<1u)|f;
                oracle_encode(raw,seed,salt,floating_nonce,&expected);
                if (obfh_arm64_upvalue_encode(raw,sizeof raw,seed,salt,
                        floating_nonce,&pkt)!=OBFH_UPVALUE_OK ||
                    pkt.nonce!=expected.nonce ||
                    pkt.floating!=expected.floating ||
                    memcmp(pkt.bytes,expected.bytes,sizeof raw)!=0)
                    return 12;
                memset(restored,0x7b,sizeof restored);
                if (obfh_arm64_upvalue_decode(&pkt,seed,salt,restored,
                        sizeof restored)!=OBFH_UPVALUE_OK ||
                    memcmp(raw,restored,sizeof raw)!=0)
                    return 13;
                digest=add_digest(digest,&pkt);
            }
        }
    }
    if (sizeof(long double)==8u &&
        digest!=UINT64_C(0x4334f9046441f6b9)) return 14;
    if (sizeof(long double)==16u &&
        digest!=UINT64_C(0xa948b84150edced9)) return 15;
    return 0;
}
static int real_long_double_bytes(void) {
    static const long double numeric_samples[]={
        0.0L,-0.0L,1.0L,-1337.25L,0.5L,1.0L/1073741824.0L
    };
    unsigned char raw[sizeof(long double)],decoded[sizeof(long double)];
    unsigned int i,j;
    for (i=0u;i<sizeof(numeric_samples)/sizeof(numeric_samples[0]);++i) {
        long double value=numeric_samples[i];
        obfh_arm64_upstream_value first,second;
        memcpy(raw,&value,sizeof raw);
        if (obfh_arm64_upvalue_encode(raw,sizeof raw,UINT32_C(0x20100031),
             16,3u,&first)!=OBFH_UPVALUE_OK ||
            obfh_arm64_upvalue_encode(raw,sizeof raw,UINT32_C(0x20100031),
             16,5u,&second)!=OBFH_UPVALUE_OK ||
            first.nonce==second.nonce) return 21;
        memset(decoded,0,sizeof decoded);
        if (obfh_arm64_upvalue_decode(&first,UINT32_C(0x20100031),16,
                                      decoded,sizeof decoded)!=OBFH_UPVALUE_OK ||
            memcmp(decoded,raw,sizeof raw)!=0) return 22;
        for (j=0u;j<4u;++j) {
            uint32_t seed=UINT32_C(0x20100031)|
                 ((j&1u)<<10u)|(((j>>1u)&1u)<<21u);
            if (obfh_arm64_upvalue_encode(raw,sizeof raw,seed,48,
                  0x12345u,&first)!=OBFH_UPVALUE_OK ||
                obfh_arm64_upvalue_decode(&first,seed,48,decoded,
                  sizeof decoded)!=OBFH_UPVALUE_OK ||
                memcmp(decoded,raw,sizeof raw)!=0) return 23;
        }
    }
    return 0;
}
static volatile uint32_t compiled_build_seed = (uint32_t)OBFH_SEED;

static int negative_admission(void) {
    const uint32_t build_seed = compiled_build_seed;
    unsigned char raw[sizeof(long double)],output[sizeof(long double)];
    obfh_arm64_upstream_value pkt,backup;
    size_t i;
    memset(raw,0xa6,sizeof raw);
    memset(&pkt,0x55,sizeof pkt);
    memcpy(&backup,&pkt,sizeof pkt);
    if (obfh_arm64_upvalue_encode(raw,sizeof raw-1u,build_seed,16,1u,&pkt)
          !=OBFH_UPVALUE_INVALID_ARGS ||
        memcmp(&pkt,&backup,sizeof pkt)!=0) return 31;
    if (obfh_arm64_upvalue_encode((const unsigned char *)0,sizeof raw,
            build_seed,16,1u,&pkt)!=OBFH_UPVALUE_INVALID_ARGS ||
        memcmp(&pkt,&backup,sizeof pkt)!=0) return 32;
    if (obfh_arm64_upvalue_encode(raw,sizeof raw,build_seed,15,1u,&pkt)
          !=OBFH_UPVALUE_INVALID_ARGS ||
        memcmp(&pkt,&backup,sizeof pkt)!=0) return 33;
    if (obfh_arm64_upvalue_encode(raw,sizeof raw,build_seed,49,1u,&pkt)
          !=OBFH_UPVALUE_INVALID_ARGS ||
        memcmp(&pkt,&backup,sizeof pkt)!=0) return 34;
    if (obfh_arm64_upvalue_encode(raw,sizeof raw,build_seed,16,1u,
            (obfh_arm64_upstream_value *)0)!=OBFH_UPVALUE_INVALID_ARGS)
        return 35;
    if (obfh_arm64_upvalue_encode(raw,sizeof raw,build_seed,16,1u,&pkt)
          !=OBFH_UPVALUE_OK) return 36;
    memset(output,0x7b,sizeof output);
    if (obfh_arm64_upvalue_decode(&pkt,build_seed,16,output,
          sizeof output-1u)!=OBFH_UPVALUE_INVALID_ARGS) return 37;
    for (i=0u;i<sizeof output;++i) if (output[i]!=0x7bu) return 38;
    pkt.floating=2u;
    if (obfh_arm64_upvalue_decode(&pkt,build_seed,16,output,
          sizeof output)!=OBFH_UPVALUE_INVALID_ARGS) return 39;
    for (i=0u;i<sizeof output;++i) if (output[i]!=0x7bu) return 40;
    if (obfh_arm64_upvalue_decode((const obfh_arm64_upstream_value *)0,
            build_seed,16,output,sizeof output)!=OBFH_UPVALUE_INVALID_ARGS)
        return 41;
    if (obfh_arm64_upvalue_decode(&pkt,build_seed,16,
            (unsigned char *)0,sizeof output)!=OBFH_UPVALUE_INVALID_ARGS)
        return 42;
    return 0;
}
int main(void) {
    int e=golden_cross_modes();
    if (e) return e;
    e=real_long_double_bytes();
    if (e) return e;
    return negative_admission();
}
