#include <atlbase.h>
#include <atlsimpcoll.h>
#include <limits.h>
/* All C++ objects/allocations remain inside this DLL. No exceptions cross C ABI. */
extern "C" __declspec(dllexport) unsigned int __cdecl poc_ratl_abi_version(void) { return 1u; }
extern "C" __declspec(dllexport) int __cdecl poc_ratl_sum(const int *input, unsigned count) {
    if (!input || count > 16) return -1;
    try {
        ATL::CSimpleArray<int> items;
        int sum = 0;
        for (unsigned i = 0; i < count; ++i) {
            if (input[i] < 0 || input[i] > 10000 || sum > 100000 - input[i]) return -2;
            if (!items.Add(input[i])) return -3;
            sum += input[i];
        }
        return sum == 12 && count == 3 && items.GetSize() == 3 ? sum : -4;
    } catch (...) { return -5; }
}
