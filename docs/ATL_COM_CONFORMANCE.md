# ATL COM P0: isolated OpenATL / RATL conformance

Tracked in [Issue #6](https://github.com/qigao/tcc/issues/6).
**Neither library is a Microsoft ATL drop-in replacement.**

- The same neutral `tests/atl_conformance/com_p0.cpp` fixture is compiled
  **twice**. Each executable gets exactly one include root: pinned MIT
  OpenATL `7b587f3e10f3f8d93a01522033de80007368bad5` or separate
  RATL `2e2efa182d7c6ee9037454d23780991fb8543084`.
- No RATL code is copied into OpenATL. No RATL/OpenATL C++ layout or
  allocation crosses a process/DLL boundary. RATL retains LGPL-2.1-or-later
  and file-specific notices; some files have GPL headers.
- P0 tests COM map with two interface offsets, canonical `IUnknown`,
  `QueryInterface` success/unsupported IID/null out-pointer errors,
  exact `AddRef/Release` accounting, `CComPtr` attach/copy/detach,
  BSTR length and a `VARIANT` integer.
- A stack-owned concrete object overrides the pure COM methods and delegates
  its QI to ATL COM maps. It intentionally does **not** qualify
  `CComObject`, Class Factory, aggregation, registration, typelib,
  `IDispatch` or real cross-apartment marshalling. Those are P1/P2.
- Both compile into Windows x64/ARM64 PE via SHA-pinned llvm-mingw. Required
  native Windows x64 and ARM64 CI must run each five times and compare a
  deterministic exact result with the expected contract; SHA256 and ARM64
  PE machine must match. Build-only success is insufficient.

No WTL8 fallback. No production Salts/compiler switch. PR #1 Draft.
