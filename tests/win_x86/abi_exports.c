#define POC_X86_EXPORT
#include "abi.h"
unsigned __cdecl poc_x86_abi_version(void) { return POC_X86_ABI_VERSION; }
unsigned __cdecl poc_x86_layout_word(void) { return POC_X86_LAYOUT_WORD; }
int __cdecl poc_x86_cdecl_add(int a, int b) { return a * 17 + b; }
int __stdcall poc_x86_stdcall_add(int a, int b) { return a * 19 + b; }
int __cdecl poc_x86_verify_payload(const poc_x86_payload *p) {
    if (!p || p->kind != 7u || p->sequence != 0x10203040u || p->last != 23u) return 0;
    return 1;
}
