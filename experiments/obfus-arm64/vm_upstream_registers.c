/* SPDX-License-Identifier: MIT
 * Source-exact four-register, flag and control-byte transport qualification.
 * Golden checks computed independently from original region-11 formulas
 * (DosX-dev/obfus.h blob 4c06fbc06f907afb3989e2851bed7583cfd0b6b0).
 * NOT a substitute for the complete numeric/long-double VM interpreter.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "obfh_arm64_upstream_registers.h"

/* Independent expression-order oracle: separate from production mix helper. */
static uint32_t reference_draw(uint32_t seed) {
    uint32_t a=seed^UINT32_C(0x6d2b79f5);
    a=(a^(a>>16u))*UINT32_C(2246822507);
    return (a^(a>>13u))*UINT32_C(3266489909);
}
static uint32_t reference_mix(uint32_t seed,uint32_t input) {
    uint32_t d=reference_draw(seed),x=input;
    unsigned int sa=11u+(d&7u),sb=12u+((d>>3u)&7u);
    uint32_t mult;
    if (d&64u) x^=x<<sa;
    else x^=x>>sa;
    switch (d%3u) {
    case 0:mult=UINT32_C(2246822507);break;
    case 1:mult=UINT32_C(3266489909);break;
    default:mult=UINT32_C(2221713035);break;
    }
    x*=mult;
    if ((d&192u)==128u) x^=x<<sb;
    else x^=x>>sb;
    return x;
}
static uint64_t digest_bytes(uint64_t hash,const unsigned char *data,size_t len) {
    size_t i;
    for (i=0u;i<len;++i) {
        hash^=data[i];
        hash*=UINT64_C(0x100000001b3);
    }
    return hash;
}
static uint64_t digest_word(uint64_t hash,uint32_t word) {
    unsigned int k;
    for (k=0u;k<4u;++k) {
        hash^=(word>>(8u*k))&255u;
        hash*=UINT64_C(0x100000001b3);
    }
    return hash;
}

static int golden_reg_flag_control(void) {
    unsigned int mode,direction,j,r,k;
    uint64_t digest=UINT64_C(0xcbf29ce484222325);
    unsigned int seen_flag_modes=0u,seen_control_sides=0u;
    if (sizeof(long double)!=8u && sizeof(long double)!=16u) return 11;
    for (mode=0u;mode<3u;++mode)
        for (direction=0u;direction<2u;++direction)
            for (j=0u;j<8u;++j) {
                obfh_upreg_state ctx;
                unsigned char input[sizeof(long double)],restored[sizeof(long double)];
                uint32_t seed=UINT32_C(0x20100031)|(mode<<8u)|
                              (direction<<27u)|(j<<16u);
                uint32_t key=UINT32_C(0x31415927)^(seed*UINT32_C(0x9e3779b9));
                uint32_t expected_steps=j*17u+mode*3u+direction;
                uint32_t draw=reference_draw(seed),flags,encoded_flags,flag_key;
                if (obfh_upreg_initialize(&ctx,seed,key)!=OBFH_UPREG_OK)
                    return 12;
                if (ctx.flags_ready!=0u || ctx.initialized!=0u)
                    return 13;
                ctx.steps=expected_steps;
                seen_flag_modes|=1u<<(draw%3u);
                seen_control_sides|=1u<<((draw&0x200u)?1u:0u);
                for (r=0u;r<4u;++r) {
                    uint32_t mask=reference_mix(seed,
                        key^(r*UINT32_C(3266489917))^expected_steps);
                    uint32_t state=mask;
                    unsigned char expected[sizeof(long double)];
                    for (k=0u;k<sizeof input;++k)
                        input[k]=(unsigned char)(k*73u+r*19u+j*11u+
                                                mode*7u+direction*31u);
                    for (k=0u;k<sizeof input;++k) {
                        size_t pos=direction ? sizeof input-1u-k : k;
                        unsigned char byte,m;
                        state=reference_mix(seed,state+(uint32_t)pos+1u);
                        byte=input[pos];
                        m=(unsigned char)state;
                        if (mode==0u) expected[pos]=(unsigned char)(byte^m);
                        else if (mode==1u)
                            expected[pos]=(unsigned char)((uint32_t)byte+m);
                        else expected[pos]=(unsigned char)((uint32_t)byte-m);
                    }
                    if (obfh_upreg_write(&ctx,r,input,sizeof input)!=OBFH_UPREG_OK ||
                        ctx.registers[r].mask!=mask ||
                        memcmp(ctx.registers[r].bytes,expected,sizeof expected)!=0 ||
                        !(ctx.initialized&(1u<<r)))
                        return 14;
                    memset(restored,0x7b,sizeof restored);
                    if (obfh_upreg_read(&ctx,r,restored,sizeof restored)!=OBFH_UPREG_OK ||
                        memcmp(restored,input,sizeof input)!=0)
                        return 15;
                    digest=digest_bytes(digest,ctx.registers[r].bytes,
                                        sizeof(long double));
                    digest=digest_word(digest,ctx.registers[r].mask);
                }
                flags=0x25u^(j*19u+mode*11u+direction*7u);
                flag_key=reference_mix(seed,key^UINT32_C(0x85ebca6b));
                if (ctx.flag_key!=flag_key ||
                    obfh_upreg_set_flags(&ctx,flags)!=OBFH_UPREG_OK ||
                    ctx.flags_ready!=0u) return 16;
                encoded_flags=draw%3u==0u ? (flags^flag_key) :
                    draw%3u==1u ? (flags+flag_key) : (flags-flag_key);
                if (ctx.flags!=encoded_flags) return 17;
                {
                    uint32_t decoded=0u;
                    if (obfh_upreg_get_flags(&ctx,&decoded)!=OBFH_UPREG_OK ||
                        decoded!=flags) return 18;
                }
                digest=digest_word(digest,ctx.flags);
                for (r=0u;r<4u;++r) {
                    uint32_t delta=r*UINT32_C(3266489917);
                    uint32_t base=(draw&0x200u) ? (key^delta) : (key+delta);
                    uint32_t mask=reference_mix(seed,base);
                    uint32_t value=UINT32_C(0xf0a05500)^
                        ((r+1u)*UINT32_C(0x1020305))^j^(mode<<16u)^(direction<<28u);
                    uint32_t decoded=0u;
                    if (obfh_upreg_control_write(&ctx,r,value)!=OBFH_UPREG_OK ||
                        ctx.controls[r]!=(mask^value) ||
                        obfh_upreg_control_read(&ctx,r,&decoded)!=OBFH_UPREG_OK ||
                        decoded!=value) return 19;
                    digest=digest_word(digest,ctx.controls[r]);
                }
                /* Source registers keep their encoded mask even when the
                 * current VM execution step counter advances. */
                ctx.steps+=33u;
                for (r=0u;r<4u;++r) {
                    for (k=0u;k<sizeof input;++k)
                        input[k]=(unsigned char)(k*73u+r*19u+j*11u+
                                                mode*7u+direction*31u);
                    if (obfh_upreg_read(&ctx,r,restored,sizeof restored)!=
                          OBFH_UPREG_OK ||
                        memcmp(restored,input,sizeof input)!=0)
                        return 20;
                }
            }
    if (seen_flag_modes!=7u || seen_control_sides!=3u) return 21;
    if (sizeof(long double)==8u &&
        digest!=UINT64_C(0x52774a6baaf5ade4)) return 22;
    if (sizeof(long double)==16u &&
        digest!=UINT64_C(0x46475a908a172544)) return 23;
    return 0;
}

static int lifecycle_and_negative(void) {
    obfh_upreg_state c,before;
    unsigned char raw[sizeof(long double)],out[sizeof(long double)];
    uint32_t v,flags;
    size_t i;
    memset(raw,0xa6,sizeof raw);
    if (obfh_upreg_initialize(&c,(uint32_t)OBFH_SEED,
                              UINT32_C(0x61551d00))!=OBFH_UPREG_OK)
        return 31;
    if (obfh_upreg_read(&c,0u,out,sizeof out)!=OBFH_UPREG_UNINITIALIZED)
        return 32;
    if (obfh_upreg_copy(&c,1u,0u)!=OBFH_UPREG_UNINITIALIZED) return 33;
    if (obfh_upreg_swap(&c,0u,1u)!=OBFH_UPREG_UNINITIALIZED) return 34;
    if (obfh_upreg_get_flags(&c,&flags)!=OBFH_UPREG_OK || flags!=0u ||
        c.flags_ready!=0u) return 35;
    for (i=0u;i<4u;++i)
        if (obfh_upreg_control_read(&c,(unsigned int)i,&v)!=OBFH_UPREG_OK ||
            v!=0u) return 36;
    if (obfh_upreg_write(&c,0u,raw,sizeof raw)!=OBFH_UPREG_OK ||
        obfh_upreg_copy(&c,1u,0u)!=OBFH_UPREG_OK ||
        obfh_upreg_copy(&c,2u,0u)!=OBFH_UPREG_OK ||
        obfh_upreg_swap(&c,1u,2u)!=OBFH_UPREG_OK)
        return 37;
    memset(out,0,sizeof out);
    if (obfh_upreg_read(&c,2u,out,sizeof out)!=OBFH_UPREG_OK ||
        memcmp(out,raw,sizeof raw)!=0) return 38;
    /* Copying encrypted storage never needs decode or writes to the source. */
    if (c.registers[0].mask!=c.registers[2].mask ||
        memcmp(c.registers[0].bytes,c.registers[2].bytes,sizeof raw)!=0)
        return 39;
    memcpy(&before,&c,sizeof c);
    if (obfh_upreg_write(&c,4u,raw,sizeof raw)!=OBFH_UPREG_INVALID_ARGS ||
        memcmp(&before,&c,sizeof c)!=0) return 40;
    if (obfh_upreg_write(&c,3u,raw,sizeof raw-1u)!=OBFH_UPREG_INVALID_ARGS ||
        memcmp(&before,&c,sizeof c)!=0) return 41;
    if (obfh_upreg_copy(&c,4u,0u)!=OBFH_UPREG_INVALID_ARGS ||
        memcmp(&before,&c,sizeof c)!=0) return 42;
    if (obfh_upreg_swap(&c,1u,3u)!=OBFH_UPREG_UNINITIALIZED ||
        memcmp(&before,&c,sizeof c)!=0) return 43;
    memset(out,0x7b,sizeof out);
    if (obfh_upreg_read(&c,3u,out,sizeof out)!=OBFH_UPREG_UNINITIALIZED)
        return 44;
    for (i=0u;i<sizeof out;++i) if (out[i]!=0x7bu) return 45;
    if (obfh_upreg_cadd(&c,0u,1u)!=OBFH_UPREG_OK ||
        obfh_upreg_control_read(&c,0u,&v)!=OBFH_UPREG_OK ||
        v!=1u || !c.flags_ready ||
        obfh_upreg_get_flags(&c,&flags)!=OBFH_UPREG_OK ||
        (flags&OBFH_UPREG_CONTROL_FLAG)==0u) return 46;
    if (obfh_upreg_cadd(&c,0u,UINT32_C(0x3ffff))!=OBFH_UPREG_OK ||
        obfh_upreg_control_read(&c,0u,&v)!=OBFH_UPREG_OK ||
        v!=0u || obfh_upreg_get_flags(&c,&flags)!=OBFH_UPREG_OK ||
        (flags&OBFH_UPREG_CONTROL_FLAG)!=0u) return 47;
    if (obfh_upreg_cxor(&c,0u,UINT32_C(0x12345))!=OBFH_UPREG_OK ||
        obfh_upreg_control_read(&c,0u,&v)!=OBFH_UPREG_OK ||
        v!=UINT32_C(0x12345)) return 48;
    if (obfh_upreg_crol(&c,0u,0u)!=OBFH_UPREG_OK ||
        obfh_upreg_control_read(&c,0u,&v)!=OBFH_UPREG_OK ||
        v!=(UINT32_C(0x12345)<<1u)) return 49;
    memcpy(&before,&c,sizeof c);
    if (obfh_upreg_cadd(&c,0u,UINT32_C(0x40000))!=OBFH_UPREG_INVALID_ARGS ||
        obfh_upreg_cxor(&c,4u,1u)!=OBFH_UPREG_INVALID_ARGS ||
        obfh_upreg_crol(&c,4u,1u)!=OBFH_UPREG_INVALID_ARGS ||
        memcmp(&before,&c,sizeof c)!=0) return 50;
    return 0;
}

int main(void) {
    int e=golden_reg_flag_control();
    if (e) return e;
    return lifecycle_and_negative();
}
