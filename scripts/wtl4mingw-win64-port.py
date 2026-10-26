#!/usr/bin/env python3
"""Fail-closed source-port transform for pinned WTL4MinGW WTL8/Winx ATL.

This runs only in a detached, exact-SHA CI checkout. It neither vendors nor
overwrites upstream source. Keep the x86 path as a regression baseline.
"""
import argparse
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument("checkout", type=Path)
args = p.parse_args()
root = args.checkout.resolve()
if not (root / "atlport/include/atlwin.h").is_file():
    p.error("WTL4MinGW checkout missing")
if not (root / "wtl/include/atlapp.h").is_file():
    p.error("WTL8 checkout missing")

def change(source: str, old: str, new: str, count: int = 1) -> str:
    actual = source.count(old)
    if actual != count:
        raise SystemExit(f"Expected {count} matches, found {actual}: {old[:100]!r}")
    return source.replace(old, new)

base = root / "atlport/include/atlbase.h"
s = base.read_text(encoding="utf-8")
s = change(s, "(IUnknown*)((int)pThis+pEntries->dw)",
           "(IUnknown*)((unsigned char*)pThis + pEntries->dw)", 2)
s = change(s, "LOWORD((DWORD)lpszDesc)", "LOWORD((ULONG_PTR)lpszDesc)")
base.write_text(s, encoding="utf-8")

atlwin = root / "atlport/include/atlwin.h"
s = atlwin.read_text(encoding="utf-8")
# Never use 32-bit SetWindowLong to store a native WNDPROC, DLGPROC or
# message result. This updates both window and contained-window code paths.
changes = [
    ("(WNDPROC)&(pThis->m_thunk.thunk)", "pThis->m_thunk.GetWNDPROC()", 3),
    ("(WNDPROC)&m_thunk.thunk", "m_thunk.GetWNDPROC()", 1),
    ("(WNDPROC)&(m_thunk.thunk)", "m_thunk.GetWNDPROC()", 3),
    ("(WNDPROC)::SetWindowLong(hWnd, GWL_WNDPROC, (LONG)pProc)",
     "(WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc)", 4),
    ("(WNDPROC)::SetWindowLong(hWnd, DWL_DLGPROC, (LONG)pProc)",
     "(WNDPROC)::SetWindowLongPtr(hWnd, DWLP_DLGPROC, (LONG_PTR)pProc)", 1),
    ("LONG pfnWndProc = ::GetWindowLong(pThis->m_hWnd, GWL_WNDPROC)",
     "LONG_PTR pfnWndProc = ::GetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC)", 2),
    ("::GetWindowLong(pThis->m_hWnd, GWL_WNDPROC) == pfnWndProc",
     "::GetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC) == pfnWndProc", 2),
    ("::SetWindowLong(pThis->m_hWnd, GWL_WNDPROC, (LONG)pThis->m_pfnSuperWindowProc)",
     "::SetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC, (LONG_PTR)pThis->m_pfnSuperWindowProc)", 2),
    ("(WNDPROC)::GetWindowLong(m_hWnd, GWL_WNDPROC)",
     "(WNDPROC)::GetWindowLongPtr(m_hWnd, GWLP_WNDPROC)", 2),
    ("::SetWindowLong(m_hWnd, GWL_WNDPROC, (LONG)m_pfnSuperWindowProc)",
     "::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_pfnSuperWindowProc)", 2),
    ("::SetWindowLong(pThis->m_hWnd, DWL_MSGRESULT, lRes)",
     "::SetWindowLongPtr(pThis->m_hWnd, DWLP_MSGRESULT, (LONG_PTR)lRes)", 1),
    ("WCHAR m_szAutoName[13];", "WCHAR m_szAutoName[32];", 1),
    ("CHAR m_szAutoName[13];", "CHAR m_szAutoName[32];", 1),
    ("(LPCTSTR)MAKELONG(atom, 0)", "(LPCTSTR)MAKEINTATOM(atom)", 2),
    ("nID = (UINT)this;", "nID = (UINT_PTR)this;", 2),
    ("UINT nID, ATOM atom", "UINT_PTR nID, ATOM atom", 2),
    ("UINT nID = 0", "UINT_PTR nID = 0", 7),
]
for old, new, count in changes:
    s = change(s, old, new, count)
# Winx's auto-generated class identifier used a 32-bit truncation of the
# WNDCLASSEX address in a 13-character buffer. Windows x64/ARM64 needs
# the entire pointer plus the ATL: prefix; bound all writes to 32 chars.
s = change(s,
    'wsprintfA(p->m_szAutoName, "ATL:%8.8X", (DWORD)&p->m_wc);',
    'int n = ::snprintf(p->m_szAutoName, sizeof(p->m_szAutoName), '
    '"ATL:%016llX", (unsigned long long)(ULONG_PTR)&p->m_wc);\n'
    '\t\t\t\tif (n < 0 || n >= (int)sizeof(p->m_szAutoName)) '
    '{ ::LeaveCriticalSection(&pM->m_csWindowCreate); return 0; }')
s = change(s,
    'wsprintfW(p->m_szAutoName, L"ATL:%8.8X", (DWORD)&p->m_wc);',
    'int n = ::swprintf(p->m_szAutoName, '
    'sizeof(p->m_szAutoName)/sizeof(p->m_szAutoName[0]), '
    'L"ATL:%016llX", (unsigned long long)(ULONG_PTR)&p->m_wc);\n'
    '\t\t\t\tif (n < 0 || n >= '
    '(int)(sizeof(p->m_szAutoName)/sizeof(p->m_szAutoName[0]))) '
    '{ ::LeaveCriticalSection(&pM->m_csWindowCreate); return 0; }')
s = change(s,
    '#ifndef __ATLBASE_H__\n\t#error atlwin.h requires atlbase.h to be included first\n#endif',
    '#ifndef __ATLBASE_H__\n\t#error atlwin.h requires atlbase.h to be included first\n#endif'
    '\n#include <stdio.h>\n#include <wchar.h>')
# _AtlCreateWndData is used by Winx to transfer ownership into the
# first native window message. Once consumed, the same union slot may be
# used by the code thunk. The RX allocation itself is a separate owner.
start = s.index("// WindowProc thunks")
end = s.index("// CMessageMap - abstract", start)
# Keep the heading before the replaced implementation and following divider.
start = s.rfind("/////////////////////////////////////////////////////////////////////////////", 0, start)
end = s.rfind("/////////////////////////////////////////////////////////////////////////////", 0, end)
thunk = """/////////////////////////////////////////////////////////////////////////////
// Architecture-neutral Win32 window-proc thunk with W^X (test-only).
// One stable owner per window; no instruction is executed out of writable
// pages. Compilation is intentionally limited to i686, amd64, and arm64.
//
struct _WndProcThunk { BYTE unused[32]; };

class CWndProcThunk {
public:
    union {
        _AtlCreateWndData cd;
        _WndProcThunk thunk;
    };

private:
    void* m_code;
    CWndProcThunk(const CWndProcThunk&);
    CWndProcThunk& operator=(const CWndProcThunk&);

public:
    CWndProcThunk() : m_code(NULL) {}
    ~CWndProcThunk() {
        if (m_code != NULL) {
            ::VirtualFree(m_code, 0, MEM_RELEASE);
            m_code = NULL;
        }
    }

    BOOL Init(WNDPROC proc, void* pThis) {
        if (!proc || !pThis || m_code != NULL) return FALSE;
        BYTE code[32] = {0};
        SIZE_T codeSize = 0;
#if defined(_M_IX86) || defined(__i386__)
        // mov dword ptr [esp+4],this; mov eax,proc; jmp eax
        // No 32-bit relative displacement constraints across allocations.
        code[0] = 0xC7; code[1] = 0x44;
        code[2] = 0x24; code[3] = 0x04;
        ::memcpy(code + 4, &pThis, 4);
        code[8] = 0xB8;
        ::memcpy(code + 9, &proc, 4);
        code[13] = 0xFF; code[14] = 0xE0;
        codeSize = 15;
#elif defined(_M_AMD64) || defined(__x86_64__)
        // Windows x64: RCX is HWND. Tail-jump to the C++ static WindowProc.
        code[0] = 0x48; code[1] = 0xB9;
        ::memcpy(code + 2, &pThis, 8);
        code[10] = 0x48; code[11] = 0xB8;
        ::memcpy(code + 12, &proc, 8);
        code[20] = 0xFF; code[21] = 0xE0;
        codeSize = 22;
#elif defined(_M_ARM64) || defined(__aarch64__)
        // Windows ARM64: ldr x0,[pc,#16]; ldr x16,[pc,#20];
        // br x16; nop; followed by aligned 64-bit this and proc literals.
        const DWORD op[4] = {
            0x58000080u, 0x580000B0u, 0xD61F0200u, 0xD503201Fu
        };
        ::memcpy(code, op, 16);
        ::memcpy(code + 16, &pThis, 8);
        ::memcpy(code + 24, &proc, 8);
        codeSize = 32;
#else
#error Unsupported Windows architecture for WTL thunk
#endif
        void *mem = ::VirtualAlloc(NULL, 4096, MEM_RESERVE | MEM_COMMIT,
                                   PAGE_READWRITE);
        if (!mem) return FALSE;
        ::memcpy(mem, code, codeSize);
        DWORD oldProtect = 0;
        if (!::VirtualProtect(mem, 4096, PAGE_EXECUTE_READ, &oldProtect) ||
            !::FlushInstructionCache(::GetCurrentProcess(), mem, codeSize)) {
            ::VirtualFree(mem, 0, MEM_RELEASE);
            return FALSE;
        }
        m_code = mem;
        return TRUE;
    }
    WNDPROC GetWNDPROC() const {
        return reinterpret_cast<WNDPROC>(m_code);
    }
};

/////////////////////////////////////////////////////////////////////////////
"""
s = s[:start] + thunk + s[end:]
# Every thunk creation must refuse admission on VirtualAlloc/VirtualProtect
# failure before trying to call or install any invalid WNDPROC address.
s = change(s,
    "pThis->m_thunk.Init(pThis->GetWindowProc(), pThis);",
    "if (!pThis->m_thunk.Init(pThis->GetWindowProc(), pThis)) return 0;")
s = change(s,
    "pThis->m_thunk.Init(pThis->GetDialogProc(), pThis);",
    "if (!pThis->m_thunk.Init(pThis->GetDialogProc(), pThis)) return 0;")
s = change(s,
    "pThis->m_thunk.Init(WindowProc, pThis);",
    "if (!pThis->m_thunk.Init(WindowProc, pThis)) return 0;")
s = change(s,
    "m_thunk.Init(GetWindowProc(), this);",
    "if (!m_thunk.Init(GetWindowProc(), this)) return FALSE;")
s = change(s,
    "m_thunk.Init(WindowProc, this);",
    "if (!m_thunk.Init(WindowProc, this)) return FALSE;")
s = change(s,
    "pThis->m_thunk.GetWNDPROC()", "pThis->m_thunk.GetWNDPROC()", 3)
atlwin.write_text(s, encoding="utf-8")

# The old Winx debug CRT shim assumes unsigned int == size_t and shadows
# the modern MinGW CRT definition. On Win64, the first operator-new
# argument must be size_t. Keep the exact x86 behavior unchanged.
crtdbg = root / "stdext/include/stdext/msvc/crtdbg.h"
s = crtdbg.read_text(encoding="utf-8")
s = change(s, "operator new(unsigned int s, int, const char *, int)",
           "operator new(size_t s, int, const char *, int)")
s = change(s, "operator new(\n        unsigned int,\n        int,",
           "operator new(\n        size_t,\n        int,")
crtdbg.write_text(s, encoding="utf-8")
print("Winx ATL port: W^X thunk, pointer-sized Win32 and size_t CRT applied")
