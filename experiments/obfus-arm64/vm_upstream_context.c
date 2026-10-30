/* SPDX-License-Identifier: MIT
 * Physical C ABI layout qualification for all four original OBFH_V_CONTEXT
 * permutations, across both long-double object sizes and three compilers.
 * Checks independently computed C field alignment and offsets, plus exact
 * byte-preserving semantic-state export/import without a second interpreter.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "obfh_arm64_upstream_context.h"

static size_t align_up(size_t n,size_t a) {
    return ((n+a-1u)/a)*a;
}

static int test_layout_0(void) {
    static const unsigned char src_order[11] = { 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u };
    const size_t got[11] = {
        offsetof(obfh_upcontext_layout_0,registers),
        offsetof(obfh_upcontext_layout_0,controls),
        offsetof(obfh_upcontext_layout_0,flags),
        offsetof(obfh_upcontext_layout_0,flag_key),
        offsetof(obfh_upcontext_layout_0,pc),
        offsetof(obfh_upcontext_layout_0,steps),
        offsetof(obfh_upcontext_layout_0,initialized),
        offsetof(obfh_upcontext_layout_0,key),
        offsetof(obfh_upcontext_layout_0,status),
        offsetof(obfh_upcontext_layout_0,flags_ready),
        offsetof(obfh_upcontext_layout_0,result)
    };
    const size_t sizes[11] = {
        sizeof(((obfh_upcontext_layout_0*)0)->registers),
        sizeof(((obfh_upcontext_layout_0*)0)->controls),
        sizeof(((obfh_upcontext_layout_0*)0)->flags),
        sizeof(((obfh_upcontext_layout_0*)0)->flag_key),
        sizeof(((obfh_upcontext_layout_0*)0)->pc),
        sizeof(((obfh_upcontext_layout_0*)0)->steps),
        sizeof(((obfh_upcontext_layout_0*)0)->initialized),
        sizeof(((obfh_upcontext_layout_0*)0)->key),
        sizeof(((obfh_upcontext_layout_0*)0)->status),
        sizeof(((obfh_upcontext_layout_0*)0)->flags_ready),
        sizeof(((obfh_upcontext_layout_0*)0)->result)
    };
    const size_t alignments[11] = { _Alignof(obfh_upreg_slot), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(long double) };
    size_t expected[11] = {0u};
    size_t cursor=0u, max_align=1u, j;
    obfh_upcontext_layout_0 physical;
    obfh_upreg_state from,restored;
    uint32_t pc=0u,status=0u;
    long double result= -0.0L,readback=1.0L;
    unsigned char bytes[sizeof(long double)],out[sizeof(long double)];
    unsigned int r,k;
    for (j=0u;j<11u;++j) {
        unsigned int slot=src_order[j];
        if (alignments[slot]>max_align) max_align=alignments[slot];
        cursor=align_up(cursor,alignments[slot]);
        expected[slot]=cursor;
        cursor+=sizes[slot];
    }
    for (j=0u;j<11u;++j) if (got[j]!=expected[j]) return 11+0*10;
    if (sizeof(physical)!=align_up(cursor,max_align) ||
        _Alignof(obfh_upcontext_layout_0)!=max_align)
        return 12+0*10;

    if (obfh_upreg_initialize(&from,
            (uint32_t)OBFH_SEED ^ (0u*UINT32_C(0x31415927)),
            UINT32_C(0x83a4cb55)+0u)!=OBFH_UPREG_OK) return 13+0*10;
    from.steps=37u+0u;
    from.flags_ready=1u;
    for (r=0u;r<4u;++r) {
        for (k=0u;k<sizeof bytes;++k)
            bytes[k]=(unsigned char)((k*43u) ^ (r*17u+0u));
        if (obfh_upreg_write(&from,r,bytes,sizeof bytes)!=OBFH_UPREG_OK ||
            obfh_upreg_control_write(&from,r,
              UINT32_C(0x1a2b3c4d)+r+0u)!=OBFH_UPREG_OK)
            return 14+0*10;
    }
    if (obfh_upreg_set_flags(&from,UINT32_C(0x29)+0u)!=OBFH_UPREG_OK)
        return 15+0*10;
    if (!obfh_upcontext_export_0(&physical,&from,
            UINT32_C(0x10203)+0u,UINT32_C(0x44)+0u,&result))
        return 16+0*10;
    if (!obfh_upcontext_import_0(&restored,&pc,&status,&readback,
            &physical,from.seed)) return 17+0*10;
    if (pc!=UINT32_C(0x10203)+0u ||
        status!=UINT32_C(0x44)+0u ||
        memcmp(&result,&readback,sizeof result)!=0 ||
        memcmp(&from,&restored,sizeof from)!=0)
        return 18+0*10;

    /* The byte register's saved encoding mask must survive a step change. */
    restored.steps+=UINT32_C(100);
    for (r=0u;r<4u;++r) {
        for (k=0u;k<sizeof bytes;++k)
            bytes[k]=(unsigned char)((k*43u) ^ (r*17u+0u));
        if (obfh_upreg_read(&restored,r,out,sizeof out)!=OBFH_UPREG_OK ||
            memcmp(bytes,out,sizeof bytes)!=0)
            return 19+0*10;
    }
    if (obfh_upcontext_export_0(((obfh_upcontext_layout_0*)0),&from,0u,0u,&result) ||
        obfh_upcontext_import_0(&restored,&pc,&status,&readback,
                    (const obfh_upcontext_layout_0*)0,from.seed))
        return 20+0*10;
    return 0;
}


static int test_layout_1(void) {
    static const unsigned char src_order[11] = { 10u, 7u, 6u, 5u, 4u, 9u, 8u, 3u, 2u, 1u, 0u };
    const size_t got[11] = {
        offsetof(obfh_upcontext_layout_1,registers),
        offsetof(obfh_upcontext_layout_1,controls),
        offsetof(obfh_upcontext_layout_1,flags),
        offsetof(obfh_upcontext_layout_1,flag_key),
        offsetof(obfh_upcontext_layout_1,pc),
        offsetof(obfh_upcontext_layout_1,steps),
        offsetof(obfh_upcontext_layout_1,initialized),
        offsetof(obfh_upcontext_layout_1,key),
        offsetof(obfh_upcontext_layout_1,status),
        offsetof(obfh_upcontext_layout_1,flags_ready),
        offsetof(obfh_upcontext_layout_1,result)
    };
    const size_t sizes[11] = {
        sizeof(((obfh_upcontext_layout_1*)0)->registers),
        sizeof(((obfh_upcontext_layout_1*)0)->controls),
        sizeof(((obfh_upcontext_layout_1*)0)->flags),
        sizeof(((obfh_upcontext_layout_1*)0)->flag_key),
        sizeof(((obfh_upcontext_layout_1*)0)->pc),
        sizeof(((obfh_upcontext_layout_1*)0)->steps),
        sizeof(((obfh_upcontext_layout_1*)0)->initialized),
        sizeof(((obfh_upcontext_layout_1*)0)->key),
        sizeof(((obfh_upcontext_layout_1*)0)->status),
        sizeof(((obfh_upcontext_layout_1*)0)->flags_ready),
        sizeof(((obfh_upcontext_layout_1*)0)->result)
    };
    const size_t alignments[11] = { _Alignof(obfh_upreg_slot), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(long double) };
    size_t expected[11] = {0u};
    size_t cursor=0u, max_align=1u, j;
    obfh_upcontext_layout_1 physical;
    obfh_upreg_state from,restored;
    uint32_t pc=0u,status=0u;
    long double result= -0.0L,readback=1.0L;
    unsigned char bytes[sizeof(long double)],out[sizeof(long double)];
    unsigned int r,k;
    for (j=0u;j<11u;++j) {
        unsigned int slot=src_order[j];
        if (alignments[slot]>max_align) max_align=alignments[slot];
        cursor=align_up(cursor,alignments[slot]);
        expected[slot]=cursor;
        cursor+=sizes[slot];
    }
    for (j=0u;j<11u;++j) if (got[j]!=expected[j]) return 11+1*10;
    if (sizeof(physical)!=align_up(cursor,max_align) ||
        _Alignof(obfh_upcontext_layout_1)!=max_align)
        return 12+1*10;

    if (obfh_upreg_initialize(&from,
            (uint32_t)OBFH_SEED ^ (1u*UINT32_C(0x31415927)),
            UINT32_C(0x83a4cb55)+1u)!=OBFH_UPREG_OK) return 13+1*10;
    from.steps=37u+1u;
    from.flags_ready=1u;
    for (r=0u;r<4u;++r) {
        for (k=0u;k<sizeof bytes;++k)
            bytes[k]=(unsigned char)((k*43u) ^ (r*17u+1u));
        if (obfh_upreg_write(&from,r,bytes,sizeof bytes)!=OBFH_UPREG_OK ||
            obfh_upreg_control_write(&from,r,
              UINT32_C(0x1a2b3c4d)+r+1u)!=OBFH_UPREG_OK)
            return 14+1*10;
    }
    if (obfh_upreg_set_flags(&from,UINT32_C(0x29)+1u)!=OBFH_UPREG_OK)
        return 15+1*10;
    if (!obfh_upcontext_export_1(&physical,&from,
            UINT32_C(0x10203)+1u,UINT32_C(0x44)+1u,&result))
        return 16+1*10;
    if (!obfh_upcontext_import_1(&restored,&pc,&status,&readback,
            &physical,from.seed)) return 17+1*10;
    if (pc!=UINT32_C(0x10203)+1u ||
        status!=UINT32_C(0x44)+1u ||
        memcmp(&result,&readback,sizeof result)!=0 ||
        memcmp(&from,&restored,sizeof from)!=0)
        return 18+1*10;

    /* The byte register's saved encoding mask must survive a step change. */
    restored.steps+=UINT32_C(100);
    for (r=0u;r<4u;++r) {
        for (k=0u;k<sizeof bytes;++k)
            bytes[k]=(unsigned char)((k*43u) ^ (r*17u+1u));
        if (obfh_upreg_read(&restored,r,out,sizeof out)!=OBFH_UPREG_OK ||
            memcmp(bytes,out,sizeof bytes)!=0)
            return 19+1*10;
    }
    if (obfh_upcontext_export_1(((obfh_upcontext_layout_1*)0),&from,0u,0u,&result) ||
        obfh_upcontext_import_1(&restored,&pc,&status,&readback,
                    (const obfh_upcontext_layout_1*)0,from.seed))
        return 20+1*10;
    return 0;
}


static int test_layout_2(void) {
    static const unsigned char src_order[11] = { 1u, 4u, 7u, 8u, 2u, 5u, 3u, 9u, 6u, 10u, 0u };
    const size_t got[11] = {
        offsetof(obfh_upcontext_layout_2,registers),
        offsetof(obfh_upcontext_layout_2,controls),
        offsetof(obfh_upcontext_layout_2,flags),
        offsetof(obfh_upcontext_layout_2,flag_key),
        offsetof(obfh_upcontext_layout_2,pc),
        offsetof(obfh_upcontext_layout_2,steps),
        offsetof(obfh_upcontext_layout_2,initialized),
        offsetof(obfh_upcontext_layout_2,key),
        offsetof(obfh_upcontext_layout_2,status),
        offsetof(obfh_upcontext_layout_2,flags_ready),
        offsetof(obfh_upcontext_layout_2,result)
    };
    const size_t sizes[11] = {
        sizeof(((obfh_upcontext_layout_2*)0)->registers),
        sizeof(((obfh_upcontext_layout_2*)0)->controls),
        sizeof(((obfh_upcontext_layout_2*)0)->flags),
        sizeof(((obfh_upcontext_layout_2*)0)->flag_key),
        sizeof(((obfh_upcontext_layout_2*)0)->pc),
        sizeof(((obfh_upcontext_layout_2*)0)->steps),
        sizeof(((obfh_upcontext_layout_2*)0)->initialized),
        sizeof(((obfh_upcontext_layout_2*)0)->key),
        sizeof(((obfh_upcontext_layout_2*)0)->status),
        sizeof(((obfh_upcontext_layout_2*)0)->flags_ready),
        sizeof(((obfh_upcontext_layout_2*)0)->result)
    };
    const size_t alignments[11] = { _Alignof(obfh_upreg_slot), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(long double) };
    size_t expected[11] = {0u};
    size_t cursor=0u, max_align=1u, j;
    obfh_upcontext_layout_2 physical;
    obfh_upreg_state from,restored;
    uint32_t pc=0u,status=0u;
    long double result= -0.0L,readback=1.0L;
    unsigned char bytes[sizeof(long double)],out[sizeof(long double)];
    unsigned int r,k;
    for (j=0u;j<11u;++j) {
        unsigned int slot=src_order[j];
        if (alignments[slot]>max_align) max_align=alignments[slot];
        cursor=align_up(cursor,alignments[slot]);
        expected[slot]=cursor;
        cursor+=sizes[slot];
    }
    for (j=0u;j<11u;++j) if (got[j]!=expected[j]) return 11+2*10;
    if (sizeof(physical)!=align_up(cursor,max_align) ||
        _Alignof(obfh_upcontext_layout_2)!=max_align)
        return 12+2*10;

    if (obfh_upreg_initialize(&from,
            (uint32_t)OBFH_SEED ^ (2u*UINT32_C(0x31415927)),
            UINT32_C(0x83a4cb55)+2u)!=OBFH_UPREG_OK) return 13+2*10;
    from.steps=37u+2u;
    from.flags_ready=1u;
    for (r=0u;r<4u;++r) {
        for (k=0u;k<sizeof bytes;++k)
            bytes[k]=(unsigned char)((k*43u) ^ (r*17u+2u));
        if (obfh_upreg_write(&from,r,bytes,sizeof bytes)!=OBFH_UPREG_OK ||
            obfh_upreg_control_write(&from,r,
              UINT32_C(0x1a2b3c4d)+r+2u)!=OBFH_UPREG_OK)
            return 14+2*10;
    }
    if (obfh_upreg_set_flags(&from,UINT32_C(0x29)+2u)!=OBFH_UPREG_OK)
        return 15+2*10;
    if (!obfh_upcontext_export_2(&physical,&from,
            UINT32_C(0x10203)+2u,UINT32_C(0x44)+2u,&result))
        return 16+2*10;
    if (!obfh_upcontext_import_2(&restored,&pc,&status,&readback,
            &physical,from.seed)) return 17+2*10;
    if (pc!=UINT32_C(0x10203)+2u ||
        status!=UINT32_C(0x44)+2u ||
        memcmp(&result,&readback,sizeof result)!=0 ||
        memcmp(&from,&restored,sizeof from)!=0)
        return 18+2*10;

    /* The byte register's saved encoding mask must survive a step change. */
    restored.steps+=UINT32_C(100);
    for (r=0u;r<4u;++r) {
        for (k=0u;k<sizeof bytes;++k)
            bytes[k]=(unsigned char)((k*43u) ^ (r*17u+2u));
        if (obfh_upreg_read(&restored,r,out,sizeof out)!=OBFH_UPREG_OK ||
            memcmp(bytes,out,sizeof bytes)!=0)
            return 19+2*10;
    }
    if (obfh_upcontext_export_2(((obfh_upcontext_layout_2*)0),&from,0u,0u,&result) ||
        obfh_upcontext_import_2(&restored,&pc,&status,&readback,
                    (const obfh_upcontext_layout_2*)0,from.seed))
        return 20+2*10;
    return 0;
}


static int test_layout_3(void) {
    static const unsigned char src_order[11] = { 8u, 9u, 7u, 6u, 3u, 5u, 2u, 4u, 0u, 10u, 1u };
    const size_t got[11] = {
        offsetof(obfh_upcontext_layout_3,registers),
        offsetof(obfh_upcontext_layout_3,controls),
        offsetof(obfh_upcontext_layout_3,flags),
        offsetof(obfh_upcontext_layout_3,flag_key),
        offsetof(obfh_upcontext_layout_3,pc),
        offsetof(obfh_upcontext_layout_3,steps),
        offsetof(obfh_upcontext_layout_3,initialized),
        offsetof(obfh_upcontext_layout_3,key),
        offsetof(obfh_upcontext_layout_3,status),
        offsetof(obfh_upcontext_layout_3,flags_ready),
        offsetof(obfh_upcontext_layout_3,result)
    };
    const size_t sizes[11] = {
        sizeof(((obfh_upcontext_layout_3*)0)->registers),
        sizeof(((obfh_upcontext_layout_3*)0)->controls),
        sizeof(((obfh_upcontext_layout_3*)0)->flags),
        sizeof(((obfh_upcontext_layout_3*)0)->flag_key),
        sizeof(((obfh_upcontext_layout_3*)0)->pc),
        sizeof(((obfh_upcontext_layout_3*)0)->steps),
        sizeof(((obfh_upcontext_layout_3*)0)->initialized),
        sizeof(((obfh_upcontext_layout_3*)0)->key),
        sizeof(((obfh_upcontext_layout_3*)0)->status),
        sizeof(((obfh_upcontext_layout_3*)0)->flags_ready),
        sizeof(((obfh_upcontext_layout_3*)0)->result)
    };
    const size_t alignments[11] = { _Alignof(obfh_upreg_slot), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(uint32_t), _Alignof(long double) };
    size_t expected[11] = {0u};
    size_t cursor=0u, max_align=1u, j;
    obfh_upcontext_layout_3 physical;
    obfh_upreg_state from,restored;
    uint32_t pc=0u,status=0u;
    long double result= -0.0L,readback=1.0L;
    unsigned char bytes[sizeof(long double)],out[sizeof(long double)];
    unsigned int r,k;
    for (j=0u;j<11u;++j) {
        unsigned int slot=src_order[j];
        if (alignments[slot]>max_align) max_align=alignments[slot];
        cursor=align_up(cursor,alignments[slot]);
        expected[slot]=cursor;
        cursor+=sizes[slot];
    }
    for (j=0u;j<11u;++j) if (got[j]!=expected[j]) return 11+3*10;
    if (sizeof(physical)!=align_up(cursor,max_align) ||
        _Alignof(obfh_upcontext_layout_3)!=max_align)
        return 12+3*10;

    if (obfh_upreg_initialize(&from,
            (uint32_t)OBFH_SEED ^ (3u*UINT32_C(0x31415927)),
            UINT32_C(0x83a4cb55)+3u)!=OBFH_UPREG_OK) return 13+3*10;
    from.steps=37u+3u;
    from.flags_ready=1u;
    for (r=0u;r<4u;++r) {
        for (k=0u;k<sizeof bytes;++k)
            bytes[k]=(unsigned char)((k*43u) ^ (r*17u+3u));
        if (obfh_upreg_write(&from,r,bytes,sizeof bytes)!=OBFH_UPREG_OK ||
            obfh_upreg_control_write(&from,r,
              UINT32_C(0x1a2b3c4d)+r+3u)!=OBFH_UPREG_OK)
            return 14+3*10;
    }
    if (obfh_upreg_set_flags(&from,UINT32_C(0x29)+3u)!=OBFH_UPREG_OK)
        return 15+3*10;
    if (!obfh_upcontext_export_3(&physical,&from,
            UINT32_C(0x10203)+3u,UINT32_C(0x44)+3u,&result))
        return 16+3*10;
    if (!obfh_upcontext_import_3(&restored,&pc,&status,&readback,
            &physical,from.seed)) return 17+3*10;
    if (pc!=UINT32_C(0x10203)+3u ||
        status!=UINT32_C(0x44)+3u ||
        memcmp(&result,&readback,sizeof result)!=0 ||
        memcmp(&from,&restored,sizeof from)!=0)
        return 18+3*10;

    /* The byte register's saved encoding mask must survive a step change. */
    restored.steps+=UINT32_C(100);
    for (r=0u;r<4u;++r) {
        for (k=0u;k<sizeof bytes;++k)
            bytes[k]=(unsigned char)((k*43u) ^ (r*17u+3u));
        if (obfh_upreg_read(&restored,r,out,sizeof out)!=OBFH_UPREG_OK ||
            memcmp(bytes,out,sizeof bytes)!=0)
            return 19+3*10;
    }
    if (obfh_upcontext_export_3(((obfh_upcontext_layout_3*)0),&from,0u,0u,&result) ||
        obfh_upcontext_import_3(&restored,&pc,&status,&readback,
                    (const obfh_upcontext_layout_3*)0,from.seed))
        return 20+3*10;
    return 0;
}
/* The live runtime physical object, not a byte-copy-only adapter, is the
 * authoritative state owner between decoded original VM instructions.
 * Exercise all four seed-selected real physical union members in each
 * compiled native AArch64 binary, including valid PC/flags/control, stale
 * register masks, no-write-on-invalid and corrupted owner identity.
 */
static int live_physical_owner_contract(void) {
    unsigned int mode,r,k;
    for (mode=0u;mode<4u;++mode) {
        const uint32_t seed=((uint32_t)OBFH_SEED &
                             ~UINT32_C(0x30)) | (mode<<4u);
        const uint32_t key=UINT32_C(0xa5b6c700)+mode;
        obfh_upcontext_live owner,before;
        obfh_upreg_state state,restored,untouched;
        uint32_t pc=UINT32_MAX,status=UINT32_MAX,flags,ctrl;
        long double result=-0.0L,readback=1.0L;
        unsigned char raw[sizeof(long double)],read[sizeof(long double)];
        uint32_t physical_pc=UINT32_MAX;
        if (!obfh_upcontext_live_initialize(&owner,seed,key) ||
            owner.mode!=mode ||
            !obfh_upcontext_live_load(&owner,&state,&pc,&status,&readback) ||
            pc!=0u || status!=0u || state.steps!=0u ||
            state.seed!=seed || state.key!=key ||
            state.initialized!=0u || state.flags_ready!=0u)
            return 101;
        state.steps=mode+7u;
        for (r=0u;r<4u;++r) {
            for (k=0u;k<sizeof raw;++k)
                raw[k]=(unsigned char)((k*29u)^(r*47u+mode));
            if (obfh_upreg_write(&state,r,raw,sizeof raw)!=OBFH_UPREG_OK ||
                obfh_upreg_control_write(&state,r,
                          UINT32_C(0xfedcba98)+r+mode)!=OBFH_UPREG_OK)
                return 102;
        }
        if (obfh_upreg_set_flags(&state,
                   UINT32_C(0x29)+mode)!=OBFH_UPREG_OK)
            return 103;
        state.flags_ready=1u;
        if (!obfh_upcontext_live_store(&owner,&state,mode+11u,mode+21u,
                                      &result))
            return 104;
        switch(mode) {
        case 0u: physical_pc=owner.physical.mode0.pc;break;
        case 1u: physical_pc=owner.physical.mode1.pc;break;
        case 2u: physical_pc=owner.physical.mode2.pc;break;
        case 3u: physical_pc=owner.physical.mode3.pc;break;
        }
        if (physical_pc!=mode+11u ||
            !obfh_upcontext_live_load(&owner,&restored,&pc,&status,
                                      &readback) ||
            pc!=mode+11u || status!=mode+21u ||
            restored.steps!=state.steps ||
            memcmp(&result,&readback,sizeof result)!=0 ||
            memcmp(&state,&restored,sizeof state)!=0 ||
            obfh_upreg_get_flags(&restored,&flags)!=OBFH_UPREG_OK ||
            flags!=UINT32_C(0x29)+mode ||
            !restored.flags_ready)
            return 105;
        /* When VM advances, existing register masks must remain readable. */
        restored.steps+=100u;
        for (r=0u;r<4u;++r) {
            for (k=0u;k<sizeof raw;++k)
                raw[k]=(unsigned char)((k*29u)^(r*47u+mode));
            if (obfh_upreg_read(&restored,r,read,sizeof read)!=
                      OBFH_UPREG_OK ||
                memcmp(raw,read,sizeof raw)!=0 ||
                obfh_upreg_control_read(&restored,r,&ctrl)!=OBFH_UPREG_OK ||
                ctrl!=UINT32_C(0xfedcba98)+r+mode)
                return 106;
        }
        memcpy(&before,&owner,sizeof owner);
        state.seed^=1u;
        if (obfh_upcontext_live_store(&owner,&state,2u,3u,&result) ||
            memcmp(&before,&owner,sizeof owner)!=0)
            return 107;
        state.seed^=1u;
        memset(&untouched,0x5c,sizeof untouched);
        memcpy(&restored,&untouched,sizeof restored);
        pc=UINT32_C(0xdecafbadu);status=UINT32_C(0xdecafbadu);
        memset(&readback,0x5c,sizeof readback);
        owner.cookie=0u;
        if (obfh_upcontext_live_load(&owner,&restored,&pc,&status,
                                     &readback) ||
            memcmp(&restored,&untouched,sizeof restored)!=0 ||
            pc!=UINT32_C(0xdecafbadu) ||
            status!=UINT32_C(0xdecafbadu))
            return 108;
        for (k=0u;k<sizeof readback;++k)
            if (((unsigned char*)&readback)[k]!=0x5cu) return 109;
        memcpy(&owner,&before,sizeof owner);
        owner.mode=mode^1u;
        if (obfh_upcontext_live_load(&owner,&restored,&pc,&status,
                                     &readback))
            return 110;
        memcpy(&owner,&before,sizeof owner);
    }
    return 0;
}

int main(void) {
    int e;
    /* Each original physical ABI form is checked even when the current
     * build seed selects only one compile-time public typedef. */
    if ((e=test_layout_0())!=0 || (e=test_layout_1())!=0 ||
        (e=test_layout_2())!=0 || (e=test_layout_3())!=0) {
        fprintf(stderr,"OBFH_CONTEXT_LAYOUT_FAIL code=%d mode=%u ld=%u\n",
            e,(unsigned int)((OBFH_SEED>>4u)%4u),
            (unsigned int)sizeof(long double));
        return e;
    }
    /* Selected alias must be exact original seed-selected physical type. */
    if (sizeof(obfh_upcontext_selected) !=
        ((OBFH_SEED>>4u)%4u==0u?sizeof(obfh_upcontext_layout_0):
         (OBFH_SEED>>4u)%4u==1u?sizeof(obfh_upcontext_layout_1):
         (OBFH_SEED>>4u)%4u==2u?sizeof(obfh_upcontext_layout_2):
                                  sizeof(obfh_upcontext_layout_3))) return 90;
    e=live_physical_owner_contract();
    if (e) {
        fprintf(stderr,"OBFH_LIVE_CONTEXT_FAIL: code=%d\n",e);
        return e;
    }
    printf("OBFH_CONTEXT_LAYOUTS_PASS: mode=%u longdouble=%u bytes\n",
          (unsigned int)((OBFH_SEED>>4u)%4u),
          (unsigned int)sizeof(long double));
    return 0;
}
