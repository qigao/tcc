/* Pinned kkhaike/openatl C++17; only integer values cross TinyCC DLL ABI.
 * No allocation, BSTR, ATL object or C++ exception crosses the boundary.
 * Do NOT construct CWndProcThunk: upstream uses RWX executable memory. */
#include <atlbase.h>
#include <atlsimpcoll.h>
#include <limits.h>

extern "C" __declspec(dllexport) unsigned __cdecl
poc_atl_abi_version(void) { return 1u; }

extern "C" __declspec(dllexport) int __cdecl
poc_atl_probe(const int *items, unsigned count) {
    if (!items || count > 16u) return -1;
    try {
        ATL::CComBSTR name(L"OpenATL");
        if (name.Length() != 7u) return -3;
        ATL::CSimpleArray<int> values;
        int sum = 0;
        for (unsigned i = 0; i < count; ++i) {
            if (items[i] < 0 || items[i] > 10000 ||
                sum > 100000 - items[i]) return -2;
            if (!values.Add(items[i])) return -4;
            sum += items[i];
        }
        ATL::CHandle event(::CreateEventW(NULL, TRUE, FALSE, NULL));
        if (event.m_h == NULL) return -5;
        if (::WaitForSingleObject(event.m_h, 0) != WAIT_TIMEOUT) return -6;
        return values.GetSize() == static_cast<int>(count) ? sum : -7;
    } catch (...) { return -8; }
}
