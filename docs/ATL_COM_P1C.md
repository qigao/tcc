# P1-C: scoped Windows registry sandbox preflight

See [Issue #8](https://github.com/qigao/tcc/issues/8). This is a
**precondition test**, not a working OpenATL `DllRegisterServer`.

The native test runs in a fresh Windows x64/ARM64 process. It creates a
new GUID-scoped key directly under `HKEY_CURRENT_USER`; no elevation
or machine registry modification is needed.

1. Calls `RegOverridePredefKey(HKEY_CLASSES_ROOT, key)`.
2. Writes a marker **through the temporary HKCU handle only** and
   proves it is readable via HKCR before any HKCR write.
3. Confirms that unimplemented OpenATL `RegisterServer` and
   `UnregisterServer` fail closed with `E_NOTIMPL`.
4. Exercises explicit redirected registry writes as a sandbox self-test,
   verifies bytes inside HKCU, rolls back the temporary class tree and
   verifies its absence. No actual class server is installed.
5. Restores the HKCR mapping **before** deleting the temporary root,
   then confirms the HKCU scratch key no longer exists.

The test must fail if isolation, redirection, restoration, or cleanup
cannot be verified. A destructor also attempts cleanup on early failure.
Test executable is native x64 and native ARM64; Linux/macOS build PE
only. This does not qualify actual persistent COM registration,
RGS parsing, crash recovery, admin/policy behavior, full COM server,
or production safety.

Microsoft documentation explicitly states that `RegOverridePredefKey`
only affects the calling process:
https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regoverridepredefkey

The upstream MIT OpenATL source is unchanged. RATL LGPL/GPL code is
not used. PR #1 stays Draft / DO NOT MERGE.
