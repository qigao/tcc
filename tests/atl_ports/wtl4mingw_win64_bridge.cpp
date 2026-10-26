/* 64-bit WTL8/Winx ATL POC after pointer-size and W^X refactor.
 * llvm-mingw builds all C++/GUI internals; TinyCC sees only a versioned
 * C DLL ABI. No C++ exception, HWND, BSTR or allocation crosses modules.
 */
#if !defined(_WIN64)
#error "WTL4MinGW Win64 probe requires x64 or ARM64 Windows"
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

WTL::CAppModule _Module;

class PortWindow : public ATL::CWindowImpl<PortWindow, ATL::CWindow, ATL::CControlWinTraits> {
public:
    int received;
    PortWindow() : received(0) {}
    BEGIN_MSG_MAP(PortWindow)
        MESSAGE_HANDLER(WM_APP + 77, OnProbe)
    END_MSG_MAP()

    LRESULT OnProbe(UINT, WPARAM wp, LPARAM lp, BOOL& handled) {
        ++received;
        handled = TRUE;
        return (LRESULT)(wp + lp);
    }
};

class ProbeIdle : public WTL::CIdleHandler {
public:
    BOOL OnIdle() { return FALSE; }
};

extern "C" __declspec(dllexport) unsigned __cdecl
poc_atl_abi_version(void) { return 1u; }

extern "C" __declspec(dllexport) int __cdecl
poc_atl_probe(const int *values, unsigned count) {
    if (!values || count > 16u) return -1;
    int outcome = -2;
    HRESULT co = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(co)) return -2;
    HRESULT hr = _Module.Init(NULL, ::GetModuleHandle(NULL));
    if (FAILED(hr)) { ::CoUninitialize(); return -3; }
    try {
        WTL::CMessageLoop loop;
        ProbeIdle idle;
        if (!_Module.AddMessageLoop(&loop)) { outcome = -4; }
        else {
            bool good = (_Module.GetMessageLoop() == &loop);
            if (good && loop.AddIdleHandler(&idle))
                good = loop.RemoveIdleHandler(&idle) != FALSE;
            else good = false;

            int sum = 0;
            for (unsigned i = 0; i < count && good; ++i) {
                if (values[i] < 0 || values[i] > 10000 ||
                    sum > 100000 - values[i]) good = false;
                else sum += values[i];
            }
            if (good) {
                // Register/dispatch a *real* WTL CWindowImpl message map.
                // The Winx thunk must be RX, never simultaneously RWX.
                PortWindow window;
                RECT bounds = {0, 0, 24, 24};
                HWND handle = window.Create(NULL, bounds, TEXT("WTL Win64 probe"),
                                            WS_POPUP, 0, 0);
                if (!handle) good = false;
                else {
                    MEMORY_BASIC_INFORMATION page = {};
                    WNDPROC handler = window.m_thunk.GetWNDPROC();
                    good = handler &&
                           ::VirtualQuery((LPCVOID)handler, &page,
                                          sizeof(page)) == sizeof(page) &&
                           ((page.Protect & 0xff) == PAGE_EXECUTE_READ);
                    if (good) {
                        LRESULT answer = ::SendMessage(handle, WM_APP + 77,
                                                        (WPARAM)5, (LPARAM)13);
                        good = (answer == 18 && window.received == 1);
                    }
                    if (!::DestroyWindow(handle)) good = false;
                    if (window.m_hWnd != NULL) good = false;
                }
            }
            if (!_Module.RemoveMessageLoop()) good = false;
            outcome = good ? sum : -5;
        }
    } catch (...) { outcome = -6; }
    _Module.Term();
    ::CoUninitialize();
    return outcome;
}
