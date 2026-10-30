/* SPDX-License-Identifier: MIT
 * Windows ARM64 native ABI qualification INSIDE the accepted static VM test.
 * No consumer project, alternate VM, SP/LR patch, or skipped compiler.
 *
 * Three separate receipts:
 * - actual OS stack walk through three distinct TinyCC nonleaf C frames,
 *   each resolved by the Windows loader's registered .pdata table;
 * - continuable exception dispatched to a real native vectored callback
 *   on the same thread (NOT frame-based SEH propagation);
 * - kernel32 locale enumeration invoking a real C callback with LPARAM
 *   round-trip (NOT proof of COM/aggregate-return callbacks).
 *
 * SEH stack propagation and GUARD_CF remain independent blocked gates.
 */
#ifndef QIGAO_OBFH_ARM64_NATIVE_ABI_H
#define QIGAO_OBFH_ARM64_NATIVE_ABI_H
#if !defined(_WIN32)
#error "This native ABI qualification only applies to Windows ARM64"
#endif
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

typedef void *(WINAPI *obfh_native_lookup_t)(
        unsigned long long, unsigned long long *, void *);
typedef USHORT (WINAPI *obfh_native_capture_t)(
        ULONG, ULONG, PVOID *, PULONG);
typedef BOOL (WINAPI *obfh_native_locale_enum_t)(
        BOOL (CALLBACK *)(LPWSTR, DWORD, LPARAM), DWORD, LPARAM, LPVOID);

static volatile ULONG obfh_native_stack_sink;

static __attribute__((noinline)) unsigned int obfh_native_leaf(
        obfh_native_capture_t capture,void **frames,
        USHORT *captured,unsigned int cookie) {
    volatile ULONG words[17];
    unsigned int i;
    USHORT count;
    for(i=0u;i<17u;++i) words[i]=(ULONG)(cookie+i*17u);
    count=capture(0u,32u,frames,(PULONG)0);
    obfh_native_stack_sink^=words[3];
    *captured=count;
    return cookie==0x2719u && count>=4u ? 113u : 0u;
}
static __attribute__((noinline)) unsigned int obfh_native_middle(
        obfh_native_capture_t capture,void **frames,
        USHORT *captured,unsigned int cookie) {
    volatile ULONG words[19];
    unsigned int i,r;
    for(i=0u;i<19u;++i) words[i]=(ULONG)(cookie+i*7u);
    r=obfh_native_leaf(capture,frames,captured,cookie);
    obfh_native_stack_sink^=words[4];
    return r ? r+17u : 0u;
}
static __attribute__((noinline)) unsigned int obfh_native_outer(
        obfh_native_capture_t capture,void **frames,
        USHORT *captured,unsigned int cookie) {
    volatile ULONG words[21];
    unsigned int i,r;
    for(i=0u;i<21u;++i) words[i]=(ULONG)(cookie+i*3u);
    r=obfh_native_middle(capture,frames,captured,cookie);
    obfh_native_stack_sink^=words[5];
    return r ? r+29u : 0u;
}
static int obfh_native_stack_walk_contract(
        HMODULE module, obfh_native_lookup_t lookup) {
    HMODULE ntdll=GetModuleHandleA("ntdll.dll");
    obfh_native_capture_t capture;
    void *frames[32];
    void *expected[3];
    unsigned long long bases[3]={0u,0u,0u};
    void *functions[3];
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)(void *)module;
    IMAGE_NT_HEADERS64 *nt=(IMAGE_NT_HEADERS64 *)(void *)
                           ((unsigned char *)(void *)module+dos->e_lfanew);
    const IMAGE_DATA_DIRECTORY *pdata=&nt->OptionalHeader.DataDirectory[
                      IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    unsigned long long base=(unsigned long long)(uintptr_t)(void *)module;
    USHORT captured=0u;
    unsigned int i,found=0u,observed=0u;
    unsigned int result;
    if(!ntdll || !lookup) return 81;
    capture=(obfh_native_capture_t)(void *)
             GetProcAddress(ntdll,"RtlCaptureStackBackTrace");
    if(!capture) return 82;
    for(i=0u;i<32u;++i) frames[i]=(void *)0;
    result=obfh_native_outer(capture,frames,&captured,0x2719u);
    if(result!=159u || captured<4u || captured>32u) return 83;
    functions[0]=(void *)(uintptr_t)(void *)&obfh_native_leaf;
    functions[1]=(void *)(uintptr_t)(void *)&obfh_native_middle;
    functions[2]=(void *)(uintptr_t)(void *)&obfh_native_outer;
    for(i=0u;i<3u;++i) {
        uintptr_t at;
        uintptr_t first= (uintptr_t)(void *)module+pdata->VirtualAddress;
        expected[i]=lookup((unsigned long long)(uintptr_t)functions[i],
                           &bases[i],(void *)0);
        at=(uintptr_t)expected[i];
        if(!expected[i] || bases[i]!=base ||
           !pdata->VirtualAddress || pdata->Size<8u ||
           at<first || at-first+8u>pdata->Size)
            return 84;
    }
    /* Validate the actual observed OS return-PC stack chain, not just the
     * existence of three separate function table records. AArch64 call
     * return PCs point just after BL; -4 locates the call instruction.
     */
    for(i=0u;i<captured && found<3u;++i) {
        uintptr_t pc=(uintptr_t)frames[i];
        unsigned long long frame_base=0u;
        void *record;
        if(pc<base+4u ||
           pc>=base+nt->OptionalHeader.SizeOfImage) continue;
        record=lookup((unsigned long long)(pc-4u),&frame_base,(void *)0);
        if(record==expected[found] && frame_base==base) ++found;
        if(record) ++observed;
    }
    if(found!=3u) {
        fprintf(stderr,"ARM64_UNWIND_RED: captured=%u matched=%u os_table_frames=%u\n",
            (unsigned int)captured,found,observed);
        return 85;
    }
    printf("ARM64_NATIVE_UNWIND_STACKWALK_PASS: captured=%u verified_native_frames=%u "
           "pdata_entries=3\n",(unsigned int)captured,found);
    return 0;
}

#define OBFH_NATIVE_EXCEPTION_CODE ((DWORD)0xe04f4248u)
static volatile LONG obfh_native_veh_seen;
static volatile DWORD obfh_native_veh_flags;
static volatile DWORD obfh_native_veh_callback_thread;
static volatile LONG obfh_native_veh_had_context;
static DWORD obfh_native_veh_thread;
static LONG CALLBACK obfh_native_vectored_handler(
        struct _EXCEPTION_POINTERS *info) {
    /* Accept only our dedicated continuable private exception. Save all
     * other flags for a STRICT post-resume assertion. This distinguishes
     * "handler never invoked" from "event delivered but unexpected flags"
     * without accepting those unexpected flags as successful evidence.
     */
    if(!info || !info->ExceptionRecord ||
       info->ExceptionRecord->ExceptionCode!=OBFH_NATIVE_EXCEPTION_CODE)
        return EXCEPTION_CONTINUE_SEARCH;
    obfh_native_veh_flags=info->ExceptionRecord->ExceptionFlags;
    obfh_native_veh_callback_thread=GetCurrentThreadId();
    obfh_native_veh_had_context=info->ContextRecord!=(void *)0;
    ++obfh_native_veh_seen;
    return EXCEPTION_CONTINUE_EXECUTION;
}
static int obfh_native_exception_dispatch_contract(void) {
    PVOID token;
    ULONG removed;
    obfh_native_veh_seen=0;
    obfh_native_veh_flags=UINT32_MAX;
    obfh_native_veh_callback_thread=0u;
    obfh_native_veh_had_context=0;
    obfh_native_veh_thread=GetCurrentThreadId();
    token=AddVectoredExceptionHandler(1u,obfh_native_vectored_handler);
    if(!token) return 86;
    fprintf(stderr,"ARM64_VEH_GATE_BEGIN: thread=%lu handler_installed=1\n",
            (unsigned long)obfh_native_veh_thread);
    fflush(stderr);
    /* Continuable native delivery; if the OS cannot resume, this is a
     * genuine RED and we MUST NOT swallow the process exit in the CI.
     */
    RaiseException(OBFH_NATIVE_EXCEPTION_CODE,0u,0u,(const ULONG_PTR*)0);
    removed=RemoveVectoredExceptionHandler(token);
    if(!removed || obfh_native_veh_seen!=1 ||
       obfh_native_veh_flags!=0u ||
       obfh_native_veh_callback_thread!=obfh_native_veh_thread ||
       !obfh_native_veh_had_context) {
        fprintf(stderr,"ARM64_VEH_POST_RED: removed=%lu seen=%ld "
                "flags=%08lx callback_tid=%lu expected_tid=%lu "
                "has_CONTEXT=%ld\n",
                (unsigned long)removed,(long)obfh_native_veh_seen,
                (unsigned long)obfh_native_veh_flags,
                (unsigned long)obfh_native_veh_callback_thread,
                (unsigned long)obfh_native_veh_thread,
                (long)obfh_native_veh_had_context);
        return 87;
    }
    puts("ARM64_NATIVE_VECTORED_EXCEPTION_DISPATCH_PASS: one continuable "
         "same-thread exception, handler retired");
    return 0;
}
typedef struct {
    uintptr_t cookie;
    DWORD thread;
    unsigned int count;
    unsigned int saw_nonempty;
} obfh_native_locale_context;
/* Callback observations are diagnostic metadata, not fallback admission. */
static volatile unsigned int obfh_native_locale_invocations;
static volatile uintptr_t obfh_native_locale_param;
static volatile DWORD obfh_native_locale_tid;
static volatile unsigned int obfh_native_locale_bad;
static BOOL CALLBACK obfh_native_locale_callback(LPWSTR name,DWORD flags,
                                                  LPARAM cookie) {
    obfh_native_locale_context *ctx=(obfh_native_locale_context *)
            (void *)(uintptr_t)cookie;
    (void)flags;
    ++obfh_native_locale_invocations;
    obfh_native_locale_param=(uintptr_t)cookie;
    obfh_native_locale_tid=GetCurrentThreadId();
    if(!ctx || ctx->cookie!=(uintptr_t)UINT32_C(0x31ac9471) ||
       ctx->thread!=obfh_native_locale_tid || !name) {
        ++obfh_native_locale_bad;
        return FALSE;
    }
    ++ctx->count;
    /* LOCALE_ALL may enumerate the invariant locale with an EMPTY locale
     * name. Empty is legal; a null pointer or incorrect LPARAM/thread is
     * not. Require at least one nonempty ordinary locale overall.
     */
    if(name[0]) ++ctx->saw_nonempty;
    return TRUE;
}
static int obfh_native_windows_callback_contract(void) {
    HMODULE kernel=GetModuleHandleA("kernel32.dll");
    obfh_native_locale_enum_t enum_locales;
    obfh_native_locale_context ctx;
    BOOL ok;
    if(!kernel) return 88;
    enum_locales=(obfh_native_locale_enum_t)(void *)
              GetProcAddress(kernel,"EnumSystemLocalesEx");
    if(!enum_locales) return 89;
    ctx.cookie=(uintptr_t)UINT32_C(0x31ac9471);
    ctx.thread=GetCurrentThreadId();
    ctx.count=0u;
    ctx.saw_nonempty=0u;
    obfh_native_locale_invocations=0u;
    obfh_native_locale_param=0u;
    obfh_native_locale_tid=0u;
    obfh_native_locale_bad=0u;
    SetLastError(0u);
    ok=enum_locales(obfh_native_locale_callback,0u,
                    (LPARAM)(uintptr_t)(void *)&ctx,(void *)0);
    if(!ok || !ctx.count || !ctx.saw_nonempty ||
       obfh_native_locale_bad ||
       obfh_native_locale_invocations!=ctx.count) {
        fprintf(stderr,"ARM64_NATIVE_CALLBACK_RED: ok=%u count=%u nonempty=%u "
                "seen=%u bad=%u param=%llx expected=%llx "
                "tid=%lu expected_thread=%lu gle=%lu\n",
                (unsigned int)ok,ctx.count,ctx.saw_nonempty,
                (unsigned int)obfh_native_locale_invocations,
                (unsigned int)obfh_native_locale_bad,
                (unsigned long long)obfh_native_locale_param,
                (unsigned long long)(uintptr_t)(void *)&ctx,
                (unsigned long)obfh_native_locale_tid,
                (unsigned long)ctx.thread,(unsigned long)GetLastError());
        return 90;
    }
    printf("ARM64_NATIVE_WINDOWS_CALLBACK_PASS: callbacks=%u "
           "LPARAM_pointer_roundtrip=1\n",ctx.count);
    return 0;
}
static int obfh_native_windows_abi_contract(void) {
    HMODULE module=GetModuleHandleA((const char *)0);
    HMODULE kernel=GetModuleHandleA("kernel32.dll");
    obfh_native_lookup_t lookup;
    int rc;
    if(!module || !kernel) return 91;
    lookup=(obfh_native_lookup_t)(void *)
              GetProcAddress(kernel,"RtlLookupFunctionEntry");
    if(!lookup) return 92;
    rc=obfh_native_stack_walk_contract(module,lookup);
    if(rc) return rc;
    /* Prove Windows-to-TinyCC ordinary callback ABI independently BEFORE
     * attempting the continuable exception path. The latter remains RED
     * if the registered exception handler is never actually invoked.
     */
    rc=obfh_native_windows_callback_contract();
    if(rc) return rc;
    return obfh_native_exception_dispatch_contract();
}
#endif
