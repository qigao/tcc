#include "abi.h"
#include <stdio.h>
#if !defined(_WIN32) || !defined(__i386__)
#error "This test must compile and run as a 32-bit Windows x86 process"
#endif
typedef unsigned (__cdecl *u0_fn)(void);
typedef int (__cdecl *add_cdecl_fn)(int, int);
typedef int (__stdcall *add_stdcall_fn)(int, int);
typedef int (__cdecl *payload_fn)(const poc_x86_payload *);
static FARPROC resolve2(HMODULE h, const char *first, const char *second) {
    FARPROC p = GetProcAddress(h, first);
    return p ? p : GetProcAddress(h, second);
}
int main(void) {
    HMODULE dll;
    u0_fn version, layout;
    add_cdecl_fn cdecl_add;
    add_stdcall_fn stdcall_add;
    payload_fn verify;
    uintptr_t before, after;
    int c, s;
    poc_x86_payload p = {7u, 0x10203040u, 23u};
    if (sizeof(void *) != 4 || sizeof(uintptr_t) != 4) return 1;
    dll = LoadLibraryA("poc_x86_abi.dll");
    if (!dll) return 2;
    version = (u0_fn)resolve2(dll, "poc_x86_abi_version", "_poc_x86_abi_version");
    layout = (u0_fn)resolve2(dll, "poc_x86_layout_word", "_poc_x86_layout_word");
    cdecl_add = (add_cdecl_fn)resolve2(dll, "poc_x86_cdecl_add", "_poc_x86_cdecl_add");
    stdcall_add = (add_stdcall_fn)resolve2(dll, "poc_x86_stdcall_add@8", "_poc_x86_stdcall_add@8");
    if (!stdcall_add) stdcall_add = (add_stdcall_fn)GetProcAddress(dll, "poc_x86_stdcall_add");
    verify = (payload_fn)resolve2(dll, "poc_x86_verify_payload", "_poc_x86_verify_payload");
    if (!version || version() != POC_X86_ABI_VERSION ||
        !layout || layout() != POC_X86_LAYOUT_WORD ||
        !cdecl_add || !stdcall_add || !verify) return 3;
    __asm__ __volatile__("movl %%esp,%0" : "=r"(before));
    c = cdecl_add(7, 11);
    s = stdcall_add(7, 11);
    __asm__ __volatile__("movl %%esp,%0" : "=r"(after));
    if (before != after || c != 130 || s != 144 || verify(&p) != 1) return 4;
    if (!FreeLibrary(dll)) return 5;
    puts("win32-x86: cdecl/stdcall stack balance, exported names, struct layout PASS");
    return 0;
}
