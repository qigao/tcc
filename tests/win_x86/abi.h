#ifndef TCC_POC_WIN_X86_ABI_H
#define TCC_POC_WIN_X86_ABI_H
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
typedef struct poc_x86_payload {
    uint16_t kind;
    uint32_t sequence;
    uint8_t last;
} poc_x86_payload;
#define POC_X86_ABI_VERSION 1u
#define POC_X86_LAYOUT_WORD ((unsigned)sizeof(poc_x86_payload) << 16 | \
                             (unsigned)offsetof(poc_x86_payload, sequence) << 8 | \
                             (unsigned)offsetof(poc_x86_payload, last))
#if defined(__cplusplus)
extern "C" {
#endif
#ifdef POC_X86_EXPORT
#define POC_X86_VIS __declspec(dllexport)
#else
#define POC_X86_VIS
#endif
POC_X86_VIS unsigned __cdecl poc_x86_abi_version(void);
POC_X86_VIS unsigned __cdecl poc_x86_layout_word(void);
POC_X86_VIS int __cdecl poc_x86_cdecl_add(int a, int b);
POC_X86_VIS int __stdcall poc_x86_stdcall_add(int a, int b);
POC_X86_VIS int __cdecl poc_x86_verify_payload(const poc_x86_payload *p);
#if defined(__cplusplus)
}
#endif
#endif
