/* Deliberately incompatible DLL for rejecting an on-disk but untrusted ABI.
 * Test-only, never installed or used for production. */
#ifdef _WIN32
__declspec(dllexport) unsigned int __cdecl poc_win64_abi_version(void) { return 99u; }
__declspec(dllexport) unsigned int __cdecl poc_win64_storage_bytes(void) { return 8u; }
#else
#error "This is a Windows DLL fixture"
#endif
