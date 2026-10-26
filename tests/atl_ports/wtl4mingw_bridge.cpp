/* Pinned waiting4love/WTL4MinGW, x86 ONLY, separate legacy ATL port.
 * Exercise WTL::CMessageLoop registration, idle handler and ATL::CWindow.
 * All WTL/ATL C++ objects die before returning through the plain C ABI.
 * No exceptions or app window handles are passed to TinyCC.
 */
#ifndef __i386__
#error "WTL4MinGW upstream only documents 32-bit MinGW"
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0601
#endif
#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
// atlctrls.h is an optional later gate: this pinned WTL8 copy
// redeclares UNDONAMEID and tree-view classes from modern MinGW CRT.
// Here we qualify the independent WTL CAppModule / CMessageLoop first.

WTL::CAppModule _Module;

class ProbeIdle : public WTL::CIdleHandler {
public:
    BOOL OnIdle() { return FALSE; }
};

extern "C" __declspec(dllexport) unsigned __cdecl
poc_atl_abi_version(void) { return 1u; }

extern "C" __declspec(dllexport) int __cdecl
poc_atl_probe(const int *items, unsigned count) {
    if (!items || count > 16u) return -1;
    int result = -2;
    HRESULT co = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(co)) return -2;
    HRESULT hr = _Module.Init(NULL, ::GetModuleHandle(NULL));
    if (FAILED(hr)) { ::CoUninitialize(); return -3; }
    try {
        WTL::CMessageLoop loop;
        ProbeIdle idle;
        if (!_Module.AddMessageLoop(&loop)) { result = -4; }
        else {
            bool ok = _Module.GetMessageLoop() == &loop;
            if (ok && loop.AddIdleHandler(&idle)) {
                ok = loop.RemoveIdleHandler(&idle) != FALSE;
            } else { ok = false; }
            int total = 0;
            for (unsigned i = 0; i < count && ok; ++i) {
                if (items[i] < 0 || items[i] > 10000 ||
                    total > 100000 - items[i]) { ok = false; break; }
                total += items[i];
            }
            // Creates a hidden native STATIC window and exercises WTL's
            // underlying ATL wrapper, without starting a visible GUI loop.
            HWND raw = ::CreateWindowExA(0, "STATIC", "wtl", WS_POPUP,
                         0, 0, 1, 1, NULL, NULL, ::GetModuleHandle(NULL), NULL);
            if (ok && raw) {
                ATL::CWindow wrapped(raw);
                ok = (::IsWindow(wrapped.m_hWnd) != FALSE) &&
                     (::SendMessageA(raw, WM_GETTEXTLENGTH, 0, 0) == 3);
            } else { ok = false; }
            if (raw) ::DestroyWindow(raw);
            if (!_Module.RemoveMessageLoop()) ok = false;
            result = ok ? total : -5;
        }
    } catch (...) { result = -6; }
    _Module.Term();
    ::CoUninitialize();
    return result;
}
