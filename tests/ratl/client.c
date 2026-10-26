#include <windows.h>
#include <stdio.h>
typedef unsigned int (__cdecl *ratl_version_fn)(void);
typedef int (__cdecl *ratl_sum_fn)(const int *, unsigned);
int main(void) {
    HMODULE dll = LoadLibraryA("poc_ratl_bridge.dll");
    ratl_version_fn version;
    ratl_sum_fn fn;
    int inputs[3] = {3, 4, 5};
    int result;
    if (!dll) { fprintf(stderr, "LoadLibrary RATL bridge: %lu\n", GetLastError()); return 1; }
    version = (ratl_version_fn)GetProcAddress(dll, "poc_ratl_abi_version");
    fn = (ratl_sum_fn)GetProcAddress(dll, "poc_ratl_sum");
    result = fn && version && version() == 1u && fn(NULL, 3) == -1 && fn(inputs, 17) == -1 ? fn(inputs, 3) : -99;
    FreeLibrary(dll);
    if (result != 12) { fprintf(stderr, "RATL result: %d\n", result); return 2; }
    puts("Win32 C-to-RATL C++ ABI: OK");
    return 0;
}
