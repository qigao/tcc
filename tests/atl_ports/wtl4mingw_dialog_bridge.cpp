/* Modeless Win64 WTL8 dialog from an in-memory resource template.
 * No compiled .rc dependency and no HWND/C++ ABI exported to TinyCC.
 * Native WTL Winx dialog callback verifies pointer-sized DWLP_MSGRESULT.
 */
#if !defined(_WIN64)
#error "Win64 modeless WTL dialog POC only"
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
#include <atldlgs.h>
#include <stdint.h>
#include <stdio.h>

WTL::CAppModule _Module;

class NativeDialog : public ATL::CDialogImpl<NativeDialog, ATL::CWindow> {
public:
    enum { IDD = 0 };
    int init_messages;
    int custom_messages;
    int final_messages;
    NativeDialog() : init_messages(0), custom_messages(0), final_messages(0) {}
    BEGIN_MSG_MAP(NativeDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInit)
        MESSAGE_HANDLER(WM_APP + 80, OnProbe)
    END_MSG_MAP()
    LRESULT OnInit(UINT, WPARAM, LPARAM, BOOL& handled) {
        ++init_messages;
        handled = TRUE;
        return TRUE;
    }
    LRESULT OnProbe(UINT, WPARAM, LPARAM, BOOL& handled) {
        ++custom_messages;
        handled = TRUE;
        return (LRESULT)0x7ABCDEF012345678ULL;
    }
    void OnFinalMessage(HWND) {
        ++final_messages;
    }
};

#pragma pack(push, 2)
struct EmptyDialog {
    DLGTEMPLATE tpl;
    WORD menu; 
    WORD class_name;
    WORD title;
};
#pragma pack(pop)

extern "C" __declspec(dllexport) unsigned __cdecl
poc_atl_abi_version(void) { return 1u; }

extern "C" __declspec(dllexport) int __cdecl
poc_atl_probe(const int *items, unsigned count) {
    if (!items || count > 16) return -1;
    int sum = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (items[i] < 0 || items[i] > 10000 || sum > 100000 - items[i])
            return -2;
        sum += items[i];
    }
    HRESULT co = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(co)) return -3;
    HRESULT init = _Module.Init(NULL, ::GetModuleHandle(NULL));
    if (FAILED(init)) { ::CoUninitialize(); return -4; }
    bool ok = true;
    unsigned failure = 0;
    try {
        EmptyDialog model = {};
        model.tpl.style = WS_POPUP | DS_MODALFRAME | WS_CAPTION;
        model.tpl.dwExtendedStyle = 0;
        model.tpl.cdit = 0;
        model.tpl.x = 0;
        model.tpl.y = 0;
        model.tpl.cx = 50;
        model.tpl.cy = 30;
        model.menu = model.class_name = model.title = 0;
        NativeDialog dialog;
        _Module.AddCreateWndData(&dialog.m_thunk.cd,
            static_cast<ATL::CDialogImplBaseT<ATL::CWindow>*>(&dialog));
        HWND hwnd = ::CreateDialogIndirectParamA(::GetModuleHandle(NULL),
                      &model.tpl, NULL,
                      (DLGPROC)NativeDialog::StartDialogProc, 0);
        if (!hwnd || dialog.m_hWnd != hwnd || dialog.init_messages != 1) {
            ok = false; failure |= 1u;
        }
        if (hwnd) {
            ::SendMessage(hwnd, WM_APP + 80, 0, 0);
            if (dialog.custom_messages != 1) {
                ok = false; failure |= 2u;
            }
            const LONG_PTR expected = (LONG_PTR)0x7ABCDEF012345678ULL;
            if (::GetWindowLongPtr(hwnd, DWLP_MSGRESULT) != expected) {
                ok = false; failure |= 4u;
            }
            MEMORY_BASIC_INFORMATION page = {};
            WNDPROC thunk = dialog.m_thunk.GetWNDPROC();
            if (!thunk ||
                ::VirtualQuery((LPCVOID)thunk, &page, sizeof(page)) != sizeof(page) ||
                (page.Protect & 0xff) != PAGE_EXECUTE_READ) {
                ok = false; failure |= 8u;
            }
            if (!::DestroyWindow(hwnd) ||
                dialog.m_hWnd != NULL || dialog.final_messages != 1) {
                ok = false; failure |= 16u;
            }
        }
    } catch (...) { ok = false; failure |= 32u; }
    _Module.Term();
    ::CoUninitialize();
    if (!ok) {
        fprintf(stderr,"WTL8 Win64 dialog failed mask=%u Win32=%lu\n",
                failure,(unsigned long)::GetLastError());
    }
    return ok ? sum : -1000 - (int)failure;
}
