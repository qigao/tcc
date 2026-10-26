# RATL P1-D: nested registry script differential

Track evidence in [Issue #9](https://github.com/qigao/tcc/issues/9).

Two **independent Windows x64 PE** programs compile exactly the same
neutral contract fixture. One uses the unmodified RATL
`2e2efa182d7c6ee9037454d23780991fb8543084`; the other uses a
separate *temporary* copy of that precise commit with a strictly
anchored test-only `RegDeleteTreeX` correction. Neither copy is linked
to OpenATL nor redistributed inside an OpenATL artifact.

All fixed scripts are **HKCR-only**. A fresh GUID-named HKCU test key is
first created and a marker proves process-local HKCR redirection before
the script executes. Never pass external scripts: upstream RATL also
recognizes HKLM and HKCU roots.

Each platform verifies nested register/unregister, REG_SZ, REG_EXPAND_SZ,
REG_DWORD and REG_BINARY values, and `NoRemove`. Existing
`ForceRemove` and `Delete` trees have nested descendants. The unmodified
RATL should expose their survival, demonstrating the original
parent-versus-child enumeration defect; the separately patched RATL
should remove those descendants. A baseline that unexpectedly succeeds,
or a patched case that still leaves stale keys, MUST fail CI rather than
silently change golden expectations.

Windows x64 executes 5 repetitions under a per-subprocess timeout, while
Linux and macOS independently cross-compile both PE variants. The
original RATL source remains unchanged and is never mixed with OpenATL.
No RATL ARM64 or production ATL deployment is qualified; no WTL8
fallback or Salts production change is authorized.

The expected native diagnostics now include the exact upstream HRESULT
(`0x00000000`, i.e. `S_OK` despite stale descendants) and actual
nested-tree state: `stale=1` for unchanged RATL and `stale=0` for the
separate test-only fix. The test first observes real keys and asserts the
result before printing these lines. Both baselines require a clean Git
checkout of the source pin; accidental mutation is a hard error.
