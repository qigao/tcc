/* WTL4MinGW real Win64 common controls and message-map lifetime test.
 * llvm-mingw C++ DLL internal only; TinyCC client uses versioned C ABI.
 * Do not ship upstream CPL/Microsoft WTL headers until legal review.
 */
#if !defined(_WIN64)
#error "Test only on native Win64 x86_64/ARM64"
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
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlframe.h>
#include <commctrl.h>
#include <stdint.h>
#include <stdio.h>

WTL::CAppModule _Module;

class ControlOwner : public ATL::CWindowImpl<ControlOwner> {
public:
    unsigned events;
    ControlOwner() : events(0) {}
    BEGIN_MSG_MAP(ControlOwner)
        MESSAGE_HANDLER(WM_APP + 79, OnProbe)
    END_MSG_MAP()
    LRESULT OnProbe(UINT, WPARAM wp, LPARAM lp, BOOL& handled) {
        ++events;
        handled = TRUE;
        return (LRESULT)(wp + lp);
    }
};

extern "C" __declspec(dllexport) unsigned __cdecl
poc_atl_abi_version(void) { return 1u; }

extern "C" __declspec(dllexport) int __cdecl
poc_atl_probe(const int *items, unsigned count) {
    if (!items || count > 16u) return -1;
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
    unsigned failed = 0;
    try {
        INITCOMMONCONTROLSEX common = {sizeof(common),
            ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_STANDARD_CLASSES};
        if (!::InitCommonControlsEx(&common)) { ok = false; failed |= 1u; }
        ControlOwner owner;
        RECT rc = {0, 0, 180, 130};
        HWND parent = NULL;
        if (ok) parent = owner.Create(NULL, rc, TEXT("WTL controls"),
                                       WS_POPUP | WS_CLIPCHILDREN, 0, 0);
        if (!parent) { ok = false; failed |= 2u; }
        if (ok) {
            HWND hEdit = ::CreateWindowEx(0, TEXT("EDIT"), TEXT("original"),
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                0, 0, 160, 25, parent, (HMENU)(UINT_PTR)101,
                ::GetModuleHandle(NULL), NULL);
            HWND hButton = ::CreateWindowEx(0, TEXT("BUTTON"), TEXT("enabled"),
                WS_CHILD | BS_AUTOCHECKBOX, 0, 25, 100, 22,
                parent, (HMENU)(UINT_PTR)102, ::GetModuleHandle(NULL), NULL);
            HWND hList = ::CreateWindowEx(0, WC_LISTVIEW, TEXT(""),
                WS_CHILD | LVS_REPORT, 0, 49, 160, 42, parent,
                (HMENU)(UINT_PTR)103, ::GetModuleHandle(NULL), NULL);
            HWND hTree = ::CreateWindowEx(0, WC_TREEVIEW, TEXT(""),
                WS_CHILD | TVS_HASLINES, 0, 92, 160, 36, parent,
                (HMENU)(UINT_PTR)104, ::GetModuleHandle(NULL), NULL);
            if (!hEdit || !hButton || !hList || !hTree) { ok = false; failed |= 4u; }
            if (ok) {
                WTL::CEdit edit(hEdit);
                WTL::CButton button(hButton);
                WTL::CListViewCtrl list(hList);
                WTL::CTreeViewCtrl tree(hTree);
                char buffer[64] = {0};
                edit.SetWindowText(TEXT("text accepted"));
                if (edit.GetWindowText(buffer, 64) != 13 ||
                    ::lstrcmpA(buffer, "text accepted") != 0) { ok = false; failed |= 8u; }
                button.SetCheck(BST_CHECKED);
                if (button.GetCheck() != BST_CHECKED) { ok = false; failed |= 16u; }

                int item = list.InsertItem(0, TEXT("record"));
                if (item != 0 || list.GetItemCount() != 1) { ok = false; failed |= 32u; }
                const DWORD_PTR highData = ((DWORD_PTR)0xABCDEF01ULL << 32) |
                                            (DWORD_PTR)0x23456789u;
                if (!list.SetItemData(0, highData) ||
                    list.GetItemData(0) != highData) { ok = false; failed |= 64u; }
                HTREEITEM root = tree.InsertItem(TEXT("root"), TVI_ROOT, TVI_LAST);
                if (!root || !tree.SetItemData(root, highData) ||
                    tree.GetItemData(root) != highData) { ok = false; failed |= 128u; }

                LRESULT result = ::SendMessage(parent, WM_APP + 79,
                                               (WPARAM)15, (LPARAM)3);
                if (result != 18 || owner.events != 1) { ok = false; failed |= 256u; }
                MEMORY_BASIC_INFORMATION page = {};
                WNDPROC callback = owner.m_thunk.GetWNDPROC();
                if (!callback ||
                    ::VirtualQuery((LPCVOID)callback, &page, sizeof(page))
                        != sizeof(page) ||
                    (page.Protect & 0xff) != PAGE_EXECUTE_READ) { ok = false; failed |= 512u; }
            }
            if (!::DestroyWindow(parent)) { ok = false; failed |= 1024u; }
            if (::IsWindow(parent) || owner.m_hWnd != NULL) { ok = false; failed |= 2048u; }
        }
    } catch (...) { ok = false; failed |= 4096u; }
    _Module.Term();
    ::CoUninitialize();
    if (!ok) {
        fprintf(stderr, "WTL64 real controls failure mask=0x%08X Win32=%lu\n",
                failed, (unsigned long)::GetLastError());
    }
    return ok ? sum : -1000 - (int)failed;
}
