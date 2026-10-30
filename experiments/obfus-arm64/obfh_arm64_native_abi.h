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

/* The exception occurs below three real TinyCC nonleaf stack frames.
 * RtlVirtualUnwind uses a COPY of the OS exception CONTEXT, never the
 * live context. This qualifies actual exception-context metadata only;
 * language-specific __try/__finally and destructive cleanup remain open.
 */
typedef void *(WINAPI *obfh_native_virtual_unwind_t)(
        DWORD,unsigned long long,unsigned long long,void *,
        CONTEXT *,void **,unsigned long long *,void *);
static obfh_native_virtual_unwind_t obfh_native_virtual_unwind;
static obfh_native_lookup_t obfh_native_exception_lookup;
static void *obfh_native_exception_expected[3];
static unsigned long long obfh_native_exception_image;
static volatile LONG obfh_native_exception_unwind_error;
static volatile LONG obfh_native_exception_unwind_frames;
static volatile LONG obfh_native_exception_unwind_steps;
/* Three caller-side snapshots are taken immediately before the descending
 * calls. The real exception-context unwind must reproduce the parent's
 * nonvolatile registers, stack pointer, and low 64 bits of V8..V15.
 * Comparing only PC/SP would miss callee-save corruption in unwind codes.
 */
typedef VOID (WINAPI *obfh_native_capture_context_t)(PCONTEXT);
static obfh_native_capture_context_t obfh_native_capture_context;
static CONTEXT obfh_native_exception_checkpoints[3];
static volatile LONG obfh_native_nonvolatile_verified;
static volatile LONG obfh_native_nonvolatile_failure_register;
static int obfh_native_check_restored_context(
        const CONTEXT *actual,const CONTEXT *saved) {
    unsigned int j;
    if(actual->Sp!=saved->Sp) return 7;
    if(actual->Fp!=saved->Fp) return 8;
    for(j=19u;j<=28u;++j) {
        if(actual->X[j]!=saved->X[j]) {
            obfh_native_nonvolatile_failure_register=(LONG)j;
            return 9;
        }
    }
    /* AArch64 preserves the LOW 64 bits of V8..V15, not their full width. */
    for(j=8u;j<=15u;++j) {
        if(memcmp(&actual->V[j],&saved->V[j],sizeof(uint64_t))!=0) {
            obfh_native_nonvolatile_failure_register=(LONG)(100u+j);
            return 10;
        }
    }
    ++obfh_native_nonvolatile_verified;
    return 0;
}
static __attribute__((noinline)) unsigned int obfh_native_exception_leaf(
        unsigned int cookie) {
    volatile ULONG frame[27];
    unsigned int i;
    for(i=0u;i<27u;++i) frame[i]=(ULONG)(cookie+i*19u);
    RaiseException(OBFH_NATIVE_EXCEPTION_CODE,0u,0u,(const ULONG_PTR*)0);
    obfh_native_stack_sink^=frame[9];
    return cookie+3u;
}
static __attribute__((noinline)) unsigned int obfh_native_exception_middle(
        unsigned int cookie) {
    volatile ULONG frame[29];
    unsigned int i,r;
    for(i=0u;i<29u;++i) frame[i]=(ULONG)(cookie+i*11u);
    obfh_native_capture_context(&obfh_native_exception_checkpoints[0]);
    r=obfh_native_exception_leaf(cookie);
    obfh_native_stack_sink^=frame[7];
    return r+5u;
}
static __attribute__((noinline)) unsigned int obfh_native_exception_outer(
        unsigned int cookie) {
    volatile ULONG frame[31];
    unsigned int i,r;
    for(i=0u;i<31u;++i) frame[i]=(ULONG)(cookie+i*13u);
    obfh_native_capture_context(&obfh_native_exception_checkpoints[1]);
    r=obfh_native_exception_middle(cookie);
    obfh_native_stack_sink^=frame[5];
    return r+7u;
}
static int obfh_native_virtual_exception_walk(const CONTEXT *original) {
    CONTEXT context;
    unsigned int i,found=0u;
    if(!original || !obfh_native_virtual_unwind ||
       !obfh_native_exception_lookup) return 1;
    context=*original; /* Preserve original exception CONTEXT. */
    for(i=0u;i<48u && found<3u;++i) {
        unsigned long long base=0u;
        unsigned long long pc=(unsigned long long)context.Pc;
        unsigned long long control_pc=pc-(i ? 4u : 0u);
        unsigned long long old_sp=(unsigned long long)context.Sp;
        unsigned long long establisher=0u;
        void *handler_data=(void *)0;
        void *entry;
        int expected;
        if(pc<4u || !old_sp || (old_sp&15u)!=0u) return 2;
        entry=obfh_native_exception_lookup(control_pc,&base,(void *)0);
        expected=(entry && base==obfh_native_exception_image &&
                  entry==obfh_native_exception_expected[found]);
        if(entry) {
            obfh_native_virtual_unwind(0u,base,control_pc,entry,
                   &context,&handler_data,&establisher,(void *)0);
        } else {
            /* Leaf without .pdata: no fabricated frame size. */
            if(!context.Lr || (unsigned long long)context.Lr==pc) return 3;
            context.Pc=context.Lr;
        }
        if((unsigned long long)context.Sp<old_sp ||
           (unsigned long long)context.Pc==pc ||
           (unsigned long long)context.Pc==0u) return 4;
        if(expected) {
            /* Every qualifying TinyCC nonleaf frame reclaims stack. */
            if(!establisher ||
               (unsigned long long)context.Sp<=old_sp) return 5;
            {
                int reg_status=obfh_native_check_restored_context(
                        &context,&obfh_native_exception_checkpoints[found]);
                if(reg_status) return reg_status;
            }
            ++found;
        }
    }
    obfh_native_exception_unwind_frames=(LONG)found;
    obfh_native_exception_unwind_steps=(LONG)i;
    return found==3u ? 0 : 6;
}
static volatile LONG obfh_native_veh_seen;
static volatile DWORD obfh_native_veh_flags;
static volatile DWORD obfh_native_veh_callback_thread;
static volatile LONG obfh_native_veh_had_context;
static volatile LONG obfh_native_veh_unwind_context_ok;
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
    obfh_native_exception_unwind_error=
        obfh_native_virtual_exception_walk(info->ContextRecord);
    obfh_native_veh_unwind_context_ok=
        obfh_native_exception_unwind_error==0;

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
    obfh_native_veh_unwind_context_ok=0;
    obfh_native_exception_unwind_error=-1;
    obfh_native_exception_unwind_frames=0;
    obfh_native_exception_unwind_steps=0;
    obfh_native_nonvolatile_verified=0;
    obfh_native_nonvolatile_failure_register=-1;

    obfh_native_veh_thread=GetCurrentThreadId();
    {
        HMODULE kernel=GetModuleHandleA("kernel32.dll");
        HMODULE ntdll=GetModuleHandleA("ntdll.dll");
        unsigned long long base=0u;
        void *functions[3];
        unsigned int i;
        if(!kernel || !ntdll) return 93;
        obfh_native_exception_lookup=(obfh_native_lookup_t)(void *)
                 GetProcAddress(kernel,"RtlLookupFunctionEntry");
        obfh_native_virtual_unwind=(obfh_native_virtual_unwind_t)(void *)
                 GetProcAddress(ntdll,"RtlVirtualUnwind");
        obfh_native_capture_context=(obfh_native_capture_context_t)(void *)
                 GetProcAddress(ntdll,"RtlCaptureContext");
        if(!obfh_native_exception_lookup || !obfh_native_virtual_unwind ||
           !obfh_native_capture_context)
            return 94;
        functions[0]=(void *)(uintptr_t)(void *)&obfh_native_exception_leaf;
        functions[1]=(void *)(uintptr_t)(void *)&obfh_native_exception_middle;
        functions[2]=(void *)(uintptr_t)(void *)&obfh_native_exception_outer;
        obfh_native_exception_image=(unsigned long long)(uintptr_t)(void *)
                 GetModuleHandleA((const char *)0);
        for(i=0u;i<3u;++i) {
            obfh_native_exception_expected[i]=obfh_native_exception_lookup(
                 (unsigned long long)(uintptr_t)functions[i],&base,(void *)0);
            if(!obfh_native_exception_expected[i] ||
               base!=obfh_native_exception_image) return 95;
        }
    }

    token=AddVectoredExceptionHandler(1u,obfh_native_vectored_handler);
    if(!token) return 86;
    fprintf(stderr,"ARM64_VEH_GATE_BEGIN: thread=%lu handler_installed=1\n",
            (unsigned long)obfh_native_veh_thread);
    fflush(stderr);
    /* Continuable native delivery; if the OS cannot resume, this is a
     * genuine RED and we MUST NOT swallow the process exit in the CI.
     */
    obfh_native_capture_context(&obfh_native_exception_checkpoints[2]);
    { unsigned int observed=obfh_native_exception_outer(0x2719u);
      if(observed!=0x2728u) return 96;
    }
    removed=RemoveVectoredExceptionHandler(token);
    /* Windows adds its internal EXCEPTION_SOFTWARE_ORIGINATE (0x80)
     * to a software-raised continuable exception. This is *not* the
     * caller's EXCEPTION_NONCONTINUABLE bit. Accept no other flags.
     * The strict 0x80-only allowance is based on actual native ARM64
     * OS evidence, not a bypass of an unhandled exception.
     */
    if(!removed || obfh_native_veh_seen!=1 ||
       (obfh_native_veh_flags & ~UINT32_C(0x80))!=0u ||
       (obfh_native_veh_flags & EXCEPTION_NONCONTINUABLE)!=0u ||
       obfh_native_veh_callback_thread!=obfh_native_veh_thread ||
       !obfh_native_veh_had_context ||
       !obfh_native_veh_unwind_context_ok ||
       obfh_native_nonvolatile_verified!=3) {
        fprintf(stderr,"ARM64_VEH_POST_RED: removed=%lu seen=%ld "
                "flags=%08lx callback_tid=%lu expected_tid=%lu "
                "has_CONTEXT=%ld\n",
                (unsigned long)removed,(long)obfh_native_veh_seen,
                (unsigned long)obfh_native_veh_flags,
                (unsigned long)obfh_native_veh_callback_thread,
                (unsigned long)obfh_native_veh_thread,
                (long)obfh_native_veh_had_context);
        fprintf(stderr,"ARM64_EXCEPTION_CONTEXT_UNWIND_RED: reason=%ld frames=%ld steps=%ld\n",
                (long)obfh_native_exception_unwind_error,
                (long)obfh_native_exception_unwind_frames,
                (long)obfh_native_exception_unwind_steps);
        fprintf(stderr,"ARM64_NONVOLATILE_RESTORE_RED: verified=%ld reg=%ld\n",
                (long)obfh_native_nonvolatile_verified,
                (long)obfh_native_nonvolatile_failure_register);
        return 87;
    }
    printf("ARM64_NATIVE_EXCEPTION_NONVOLATILE_RESTORE_PASS: frames=%ld x19_x29=1 v8_v15_low64=1 sp=1\n",
           (long)obfh_native_nonvolatile_verified);
    printf("ARM64_NATIVE_EXCEPTION_CONTEXT_VIRTUAL_UNWIND_PASS: frames=%ld steps=%ld\n",
           (long)obfh_native_exception_unwind_frames,
           (long)obfh_native_exception_unwind_steps);
    printf("ARM64_NATIVE_VECTORED_EXCEPTION_DISPATCH_PASS: "
           "one continuable same-thread exception, handler retired, "
           "os_software_origin_flags=%08lx\n",
           (unsigned long)obfh_native_veh_flags);
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

/* Separate Windows ARM64 phase-two experiment.
 *
 * Unlike the earlier RtlVirtualUnwind copy check, RtlUnwindEx actually
 * transfers control from a live TinyCC nonleaf stack to the target frame.
 * A dedicated continuable RaiseException supplies the EXCEPTION_RECORD.
 * This manual unwind is NOT generated __try/__except/__finally scope cleanup.
 * Only the explicit opt-in --phase2-unwind mode executes this path.
 */
#define OBFH_NATIVE_PHASE2_EXCEPTION_CODE ((DWORD)0xe04f4249u)
typedef VOID (WINAPI *obfh_native_unwind_ex_t)(
        PVOID,PVOID,PEXCEPTION_RECORD,PVOID,PCONTEXT,PVOID);
static obfh_native_unwind_ex_t obfh_native_unwind_ex;
static void *obfh_phase2_target_frame;
static void *obfh_phase2_target_pc;
static EXCEPTION_RECORD obfh_phase2_exception_record;
static volatile LONG obfh_phase2_exception_seen;
static volatile LONG obfh_phase2_leaf_returned;
static volatile LONG obfh_phase2_middle_returned;
static volatile LONG obfh_phase2_outer_returned;
static volatile LONG obfh_phase2_resumed;
static volatile LONG obfh_phase2_error;
static unsigned long long obfh_phase2_target_sp;
static PVOID obfh_phase2_handler_token;

static LONG CALLBACK obfh_native_phase2_vectored_handler(
        struct _EXCEPTION_POINTERS *info) {
    if(!info || !info->ExceptionRecord ||
       info->ExceptionRecord->ExceptionCode!=OBFH_NATIVE_PHASE2_EXCEPTION_CODE)
        return EXCEPTION_CONTINUE_SEARCH;
    if(!info->ContextRecord ||
       (info->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE))
        return EXCEPTION_CONTINUE_SEARCH;
    obfh_phase2_exception_record=*info->ExceptionRecord;
    ++obfh_phase2_exception_seen;
    return EXCEPTION_CONTINUE_EXECUTION;
}
static __attribute__((noinline)) void obfh_native_phase2_leaf(void) {
    volatile DWORD frame[37];
    CONTEXT context;
    unsigned int i;
    for(i=0u;i<37u;++i) frame[i]=0x51740000u+i*37u;
    RaiseException(OBFH_NATIVE_PHASE2_EXCEPTION_CODE,0u,0u,
                   (const ULONG_PTR*)0);
    if(obfh_phase2_exception_seen!=1 ||
       !obfh_phase2_target_frame || !obfh_phase2_target_pc) {
        obfh_phase2_error=1;
        return;
    }
    obfh_native_capture_context(&context);
    /* Real phase-two OS unwinder; should NEVER return to this frame. */
    obfh_native_unwind_ex(obfh_phase2_target_frame,obfh_phase2_target_pc,
                          &obfh_phase2_exception_record,(void *)0,
                          &context,(void *)0);
    obfh_phase2_leaf_returned=1;
    obfh_native_stack_sink^=frame[8];
}
static __attribute__((noinline)) void obfh_native_phase2_middle(void) {
    volatile DWORD frame[43];
    unsigned int i;
    for(i=0u;i<43u;++i) frame[i]=0x62750000u+i*19u;
    obfh_native_phase2_leaf();
    obfh_phase2_middle_returned=1;
    obfh_native_stack_sink^=frame[9];
}
static __attribute__((noinline)) int obfh_native_phase2_outer(void) {
    volatile DWORD frame[47];
    CONTEXT snapshot,unwind_context,after;
    unsigned long long base=0u,establisher=0u;
    void *handler_data=(void *)0;
    void *entry;
    unsigned int i;
    for(i=0u;i<47u;++i) frame[i]=0x73860000u+i*23u;
    obfh_phase2_target_pc=&&obfh_phase2_landing;
    obfh_native_capture_context(&snapshot);
    obfh_phase2_target_sp=(unsigned long long)snapshot.Sp;
    unwind_context=snapshot;
    entry=obfh_native_exception_lookup((unsigned long long)snapshot.Pc,
                                        &base,(void *)0);
    if(!entry || !base || !obfh_phase2_target_pc) return 101;
    obfh_native_virtual_unwind(0u,base,(unsigned long long)snapshot.Pc,entry,
                &unwind_context,&handler_data,&establisher,(void *)0);
    if(!establisher || (unsigned long long)unwind_context.Sp<=snapshot.Sp)
        return 102;
    obfh_phase2_target_frame=(void *)(uintptr_t)establisher;
    obfh_native_phase2_middle();
    obfh_phase2_outer_returned=1;
    return 103; /* Ordinary nested returns MUST be skipped. */

obfh_phase2_landing:
    obfh_native_capture_context(&after);
    ++obfh_phase2_resumed;
    if(obfh_phase2_exception_seen!=1 || obfh_phase2_error ||
       obfh_phase2_leaf_returned || obfh_phase2_middle_returned ||
       obfh_phase2_outer_returned) return 104;
    if((unsigned long long)after.Sp!=obfh_phase2_target_sp ||
       (unsigned long long)after.Sp==0u ||
       frame[10]!=(DWORD)(0x73860000u+10u*23u) ||
       frame[36]!=(DWORD)(0x73860000u+36u*23u))
        return 105;
    /* Validate the *live* Windows RtlUnwindEx target context, not only
     * the earlier copy-only RtlVirtualUnwind context. Match all 10
     * nonvolatile integer registers, FP, SP and V8..V15 low halves. */
    if(obfh_native_check_restored_context(&after,&snapshot)!=0)
        return 112;
    return 0;
}
static int obfh_native_phase2_unwind_contract(void) {
    HMODULE ntdll=GetModuleHandleA("ntdll.dll");
    HMODULE kernel=GetModuleHandleA("kernel32.dll");
    int rc;
    if(!ntdll || !kernel) return 106;
    obfh_native_exception_lookup=(obfh_native_lookup_t)(void *)
            GetProcAddress(kernel,"RtlLookupFunctionEntry");
    obfh_native_virtual_unwind=(obfh_native_virtual_unwind_t)(void *)
            GetProcAddress(ntdll,"RtlVirtualUnwind");
    obfh_native_capture_context=(obfh_native_capture_context_t)(void *)
            GetProcAddress(ntdll,"RtlCaptureContext");
    obfh_native_unwind_ex=(obfh_native_unwind_ex_t)(void *)
            GetProcAddress(ntdll,"RtlUnwindEx");
    if(!obfh_native_exception_lookup || !obfh_native_virtual_unwind ||
       !obfh_native_capture_context || !obfh_native_unwind_ex) return 107;
    obfh_phase2_exception_seen=0;
    obfh_phase2_leaf_returned=0;
    obfh_phase2_middle_returned=0;
    obfh_phase2_outer_returned=0;
    obfh_phase2_resumed=0;
    obfh_phase2_error=0;
    obfh_phase2_handler_token=AddVectoredExceptionHandler(
                              1u,obfh_native_phase2_vectored_handler);
    if(!obfh_phase2_handler_token) return 108;
    rc=obfh_native_phase2_outer();
    if(!RemoveVectoredExceptionHandler(obfh_phase2_handler_token)) return 109;
    if(rc || obfh_phase2_resumed!=1 || obfh_phase2_exception_seen!=1 ||
       obfh_phase2_error || obfh_phase2_leaf_returned ||
       obfh_phase2_middle_returned || obfh_phase2_outer_returned) {
        fprintf(stderr,"ARM64_NATIVE_DESTRUCTIVE_UNWIND_RED: rc=%d resumed=%ld "
                "seen=%ld leaf=%ld middle=%ld outer=%ld err=%ld\n",
                rc,(long)obfh_phase2_resumed,(long)obfh_phase2_exception_seen,
                (long)obfh_phase2_leaf_returned,
                (long)obfh_phase2_middle_returned,
                (long)obfh_phase2_outer_returned,
                (long)obfh_phase2_error);
        return 110;
    }
    printf("ARM64_NATIVE_DESTRUCTIVE_UNWIND_PASS: target_frame=1 "
           "exception=1 resumed=1 inner_returns=0 outer_stack_preserved=1 "
           "nonvolatile_restored=1\n");
    return 0;
}

#endif
