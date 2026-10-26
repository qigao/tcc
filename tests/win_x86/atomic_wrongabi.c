/* Test-only incompatible provider. Reject before acquiring any entry points. */
#ifdef _WIN32
__declspec(dllexport) unsigned __cdecl poc_x86_atomic_version(void) {
    return 99u;
}
#else
#error "Windows x86 only"
#endif
