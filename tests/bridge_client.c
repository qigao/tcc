#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "poc/bridge.h"
typedef uint32_t (POC_CALL *version_fn)(void);
typedef uint32_t (POC_CALL *checksum_fn)(const uint8_t *, uint32_t);
typedef int32_t (POC_CALL *sum_fn)(const int32_t *, uint32_t);
int main(void) {
    const uint8_t input[] = {1, 2, 3};
    const int32_t values[] = {3, 4, 5};
    HMODULE module = LoadLibraryA("poc_bridge.dll");
    version_fn version;
    checksum_fn checksum;
    sum_fn sum;
    int ok;
    if (!module) { fprintf(stderr, "LoadLibrary poc_bridge.dll failed: %lu\n", GetLastError()); return 1; }
    version = (version_fn)GetProcAddress(module, "poc_bridge_abi_version");
    checksum = (checksum_fn)GetProcAddress(module, "poc_bridge_checksum");
    sum = (sum_fn)GetProcAddress(module, "poc_bridge_sum");
    ok = version && version() == POC_BRIDGE_ABI_VERSION && checksum && sum && sum(values, 3) == 12 && checksum(input, 3) == UINT32_C(1456420779);
    FreeLibrary(module);
    if (!ok) return 2;
    puts("Win32 C ABI: DLL export + TCC/Clang dynamic lookup OK");
    return 0;
}
