/* True Windows TinyCC C client. Never imports C++ symbols/ABI. */
#include <windows.h>
#include <stdio.h>
typedef unsigned (__cdecl *version_fn)(void);
typedef int (__cdecl *probe_fn)(const int *, unsigned);
int main(int argc, char **argv) {
    HMODULE dll;
    version_fn version;
    probe_fn probe;
    int values[3] = {3, 4, 5};
    int valid, invalid;
    if (argc != 2 || !argv[1]) return 1;
    dll = LoadLibraryA(argv[1]);
    if (!dll) {
        fprintf(stderr, "LoadLibrary %s failed: %lu\n", argv[1], GetLastError());
        return 2;
    }
    version = (version_fn)GetProcAddress(dll, "poc_atl_abi_version");
    probe = (probe_fn)GetProcAddress(dll, "poc_atl_probe");
#ifdef _M_IX86
    if (!version) version = (version_fn)GetProcAddress(dll, "_poc_atl_abi_version");
    if (!probe) probe = (probe_fn)GetProcAddress(dll, "_poc_atl_probe");
#endif
    if (!version || version() != 1u || !probe) {
        FreeLibrary(dll);
        return 3;
    }
    invalid = probe(NULL, 3);
    valid = probe(values, 3);
    if (!FreeLibrary(dll)) return 4;
    if (invalid != -1 || valid != 12) {
        fprintf(stderr, "ATL port %s returned (%d,%d)\n", argv[1], invalid, valid);
        return 5;
    }
    puts("llvm-mingw ATL/WTL C++ DLL -> TinyCC C ABI PASS");
    return 0;
}
