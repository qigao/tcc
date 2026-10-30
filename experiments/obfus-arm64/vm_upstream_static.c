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
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <stdlib.h>
#endif
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
/* Real Windows ARM64 loader/PE test, inside the SAME existing native
 * executable. This does not add a consumer, runner job, or fallback engine.
 * Separate gates remain for actual SEH propagation and CFG enforcement.
 */
#if defined(_WIN32)
static int disk_rva_offset(const unsigned char *file,size_t length,
                           const IMAGE_NT_HEADERS64 *nt,
                           DWORD rva, DWORD need, size_t *offset) {
    const IMAGE_SECTION_HEADER *sections=IMAGE_FIRST_SECTION(nt);
    unsigned int i;
    for(i=0u;i<nt->FileHeader.NumberOfSections;++i) {
        const IMAGE_SECTION_HEADER *sec=&sections[i];
        unsigned long long rel;
        if(rva<sec->VirtualAddress) continue;
        rel=(unsigned long long)rva-sec->VirtualAddress;
        if(rel+need>sec->SizeOfRawData) continue;
        if((unsigned long long)sec->PointerToRawData+rel+need>length)
            return 0;
        *offset=(size_t)sec->PointerToRawData+(size_t)rel;
        return 1;
    }
    (void)file;
    return 0;
}
static int windows_loader_relocation_and_function_table(void) {
    HMODULE module=GetModuleHandleA((const char *)0);
    unsigned char *image=(unsigned char *)(void *)module;
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS64 *nt;
    const IMAGE_DATA_DIRECTORY *reloc;
    FILE *stream;
    unsigned char *disk=(unsigned char *)0;
    long disk_size;
    char path[4096];
    DWORD directory_at=0u, validated=0u, scanned=0u;
    unsigned long long preferred=0u,actual=0u,delta=0u;
    MEMORY_BASIC_INFORMATION memory;
    unsigned long long lookup_base=0u;
    typedef void *(WINAPI *obfh_lookup_fn)(
        unsigned long long,unsigned long long *,void *);
    obfh_lookup_fn lookup;
    void *runtime_entry;
    HMODULE kernel;
    size_t checked=0u;
    int status=0;
    if(!module || !image) return 51;
    dos=(const IMAGE_DOS_HEADER *)(const void *)image;
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE ||
       dos->e_lfanew<64 || dos->e_lfanew>0x1000) return 52;
    nt=(const IMAGE_NT_HEADERS64 *)(const void *)(image+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE ||
       nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_ARM64 ||
       nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
       !(nt->OptionalHeader.DllCharacteristics&IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE) ||
       !(nt->OptionalHeader.DllCharacteristics&IMAGE_DLLCHARACTERISTICS_NX_COMPAT))
        return 53;
    actual=(unsigned long long)(uintptr_t)(void *)module;
    /* The Windows loader may rewrite the in-memory optional header's
     * ImageBase. The original preferred address MUST come from the on-disk
     * PE image, NOT from the mapped header (which may equal actual).
     */
    if(VirtualQuery((const void *)site0_words,&memory,sizeof memory)!=
                          sizeof memory ||
       memory.State!=MEM_COMMIT ||
       (memory.Protect & 0xffu)!=PAGE_READONLY) return 55;
    if(VirtualQuery((const void *)site7_words,&memory,sizeof memory)!=
                          sizeof memory ||
       memory.State!=MEM_COMMIT ||
       (memory.Protect & 0xffu)!=PAGE_READONLY) return 56;

    if(!GetModuleFileNameA((HMODULE)0,path,(DWORD)sizeof path) ||
       strlen(path)>=sizeof path-1u) return 57;
    stream=fopen(path,"rb");
    if(!stream) return 58;
    if(fseek(stream,0,SEEK_END)!=0) { fclose(stream);return 59; }
    disk_size=ftell(stream);
    if(disk_size<1024 || disk_size>16*1024*1024) {
        fclose(stream);return 60;
    }
    if(fseek(stream,0,SEEK_SET)!=0) { fclose(stream);return 61; }
    disk=(unsigned char *)malloc((size_t)disk_size);
    if(!disk) { fclose(stream);return 62; }
    checked=fread(disk,1u,(size_t)disk_size,stream);
    fclose(stream);
    if(checked!=(size_t)disk_size) { status=63;goto cleanup; }
    {
        const IMAGE_DOS_HEADER *disk_dos=(const IMAGE_DOS_HEADER *)(const void *)disk;
        const IMAGE_NT_HEADERS64 *disk_nt;
        if(disk_dos->e_magic!=IMAGE_DOS_SIGNATURE ||
           disk_dos->e_lfanew<64 ||
           (unsigned long long)disk_dos->e_lfanew+
             sizeof(IMAGE_NT_HEADERS64)>(unsigned long long)disk_size) {
            status=54;goto cleanup;
        }
        disk_nt=(const IMAGE_NT_HEADERS64 *)(const void *)
                (disk+disk_dos->e_lfanew);
        if(disk_nt->Signature!=IMAGE_NT_SIGNATURE ||
           disk_nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
           disk_nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_ARM64) {
            status=54;goto cleanup;
        }
        preferred=(unsigned long long)disk_nt->OptionalHeader.ImageBase;
        delta=actual-preferred;
        if(actual==preferred) {
            fprintf(stderr,"ARM64_LOADER_RED: still at on-disk preferred base %llx\n",
                    preferred);
            status=54;goto cleanup;
        }
    }

    reloc=&nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if(!reloc->VirtualAddress || !reloc->Size ||
       reloc->VirtualAddress>=nt->OptionalHeader.SizeOfImage ||
       reloc->Size>nt->OptionalHeader.SizeOfImage-reloc->VirtualAddress)
        { status=64;goto cleanup; }
    while(directory_at<reloc->Size) {
        const IMAGE_BASE_RELOCATION *block;
        unsigned int i,items;
        DWORD at=reloc->VirtualAddress+directory_at;
        if(reloc->Size-directory_at<8u) { status=65;goto cleanup; }
        block=(const IMAGE_BASE_RELOCATION *)(const void *)(image+at);
        if(block->SizeOfBlock<8u || block->SizeOfBlock%2u!=0u ||
           block->SizeOfBlock>reloc->Size-directory_at)
            { status=66;goto cleanup; }
        items=(block->SizeOfBlock-8u)/2u;
        for(i=0u;i<items;++i) {
            const WORD *records=(const WORD *)(const void *)(block+1);
            WORD record=records[i];
            unsigned int type=record>>12u;
            DWORD slot_rva;
            size_t file_at;
            unsigned long long original,relocated;
            if(!type) continue;
            ++scanned;
            if(type!=IMAGE_REL_BASED_DIR64) {
                fprintf(stderr,"ARM64_LOADER_RED: unexpected reloc type %u\n",type);
                status=67;goto cleanup;
            }
            slot_rva=block->VirtualAddress+(record&0xfffu);
            if(slot_rva>nt->OptionalHeader.SizeOfImage ||
               nt->OptionalHeader.SizeOfImage-slot_rva<8u ||
               !disk_rva_offset(disk,(size_t)disk_size,nt,slot_rva,8u,&file_at))
                { status=68;goto cleanup; }
            memcpy(&original,disk+file_at,sizeof original);
            memcpy(&relocated,image+slot_rva,sizeof relocated);
            if(relocated!=original+delta) {
                fprintf(stderr,
                        "ARM64_LOADER_RED: relocation RVA=%08x orig=%llx live=%llx delta=%llx\n",
                        slot_rva,original,relocated,delta);
                status=69;goto cleanup;
            }
            ++validated;
        }
        directory_at+=block->SizeOfBlock;
    }
    if(!validated || validated!=scanned) { status=70;goto cleanup; }

    /* Windows loader must register a concrete .pdata RUNTIME_FUNCTION for
     * a true nonleaf VM validation function; merely having a .pdata section
     * would not establish loader-visible unwind-table admission. */
    kernel=GetModuleHandleA("kernel32.dll");
    if(!kernel) { status=71;goto cleanup; }
    lookup=(obfh_lookup_fn)(void *)GetProcAddress(kernel,"RtlLookupFunctionEntry");
    if(!lookup) { status=72;goto cleanup; }
    runtime_entry=lookup((unsigned long long)(uintptr_t)(void *)&static_equivalence,
                         &lookup_base,(void *)0);
    if(!runtime_entry || lookup_base!=actual) { status=73;goto cleanup; }
    {
        const IMAGE_DATA_DIRECTORY *pdata=&nt->OptionalHeader.DataDirectory[
                IMAGE_DIRECTORY_ENTRY_EXCEPTION];
        uintptr_t at=(uintptr_t)runtime_entry;
        uintptr_t first=(uintptr_t)(void *)module+pdata->VirtualAddress;
        if(!pdata->VirtualAddress || pdata->Size<8u ||
           at<first || at-first+8u>pdata->Size)
            { status=74;goto cleanup; }
    }
    printf("ARM64_NATIVE_LOADER_RELOC_PDATA_PASS: loaded=%llx preferred=%llx delta=%llx "
           "DIR64=%u .pdata_lookup=1 guard_cf_advertised=%u\n",
           actual,preferred,delta,validated,
           !!(nt->OptionalHeader.DllCharacteristics&
              IMAGE_DLLCHARACTERISTICS_GUARD_CF));
cleanup:
    free(disk);
    if(status)
        fprintf(stderr,"ARM64_NATIVE_LOADER_RELOC_PDATA_RED: reason=%d loaded=%llx preferred=%llx checked=%u\n",
                status,actual,preferred,validated);
    return status;
}
/* Native stack walking, exception dispatch and Windows callback ABI are
 * validated in this same fixture, never as a new consumer/runtime engine. */
#include "obfh_arm64_native_abi.h"
#endif

int main(void) {
#if defined(_WIN32)
    int rc=windows_loader_relocation_and_function_table();
    if(rc) return rc;
    rc=obfh_native_windows_abi_contract();
    if(rc) return rc;
#else
    int rc;
#endif
    rc=static_equivalence();
    if (rc) return rc;
    rc=single_eval();
    if (rc) return rc;
    return fail_closed();
}
