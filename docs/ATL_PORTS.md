# Windows llvm-mingw ATL/WTL header interoperability

This is an independent Windows compiler compatibility POC, not a Salts
production GUI, COM component or ABI. CI clones exact immutable Git
commits; none of these third-party headers or binaries are vendored.

| Upstream | Pin | Scope |
| --- | --- | --- |
| [kkhaike/openatl](https://github.com/kkhaike/openatl) | \`7b587f3e10f3f8d93a01522033de80007368bad5\` | Modern, C++17, header-only, MIT root license; Windows llvm-mingw x64 first |
| [waiting4love/WTL4MinGW](https://github.com/waiting4love/WTL4MinGW) | \`47dc3c0ff0e44beb8674b6f59533a31c3e08f2a5\` | Historical WTL8 and Winx ATL port; **x86 only** per README; legacy diagnostics first |
| [katahiromz/RATL](https://github.com/katahiromz/RATL) | \`2e2efa182d7c6ee9037454d23780991fb8543084\` | Existing LGPL ReactOS ATL independent check; do not mix include trees |

## Compilation and ABI

- OpenATL x64 gets only \`tools/openatl/include\`; WTL4MinGW x86
  gets only \`tools/wtl4mingw/atlport/include\`,
  \`tools/wtl4mingw/stdext/include\` and
  \`tools/wtl4mingw/wtl/include\`. These are separate DLL builds
  and must not share header search order, C++ objects, allocation or
  static link objects with each other or RATL.
- Both DLLs are built by the pinned llvm-mingw C++ compiler; the actual
  Windows TinyCC x86/x64 driver compiles a C caller using only
  \`LoadLibraryA\`, versioned integer C exports and \`FreeLibrary\`.
  No exceptions, \`BSTR\`, \`HWND\`, C++ RTTI or object ownership cross
  the compiler boundary.
- OpenATL test allocates/releases \`CComBSTR\`, uses a \`CSimpleArray<int>\`
  and checks Win32 \`CHandle\` ownership. WTL4MinGW test exercises
  \`CAppModule\`, \`CMessageLoop\`, \`CIdleHandler\` and a hidden
  Win32 \`STATIC\` window wrapped by ATL::CWindow.
- Fail closed when the compiler, ABI symbols or runtime test fails;
  do not silently switch from Clang to MSVC or MinGW GCC.

## Important limitations and license review

- OpenATL is clean-room MIT according to its root LICENSE and README.
  Its current \`CWndProcThunk\` implementation allocates writable and
  executable Win32 memory (\`PAGE_EXECUTE_READWRITE\`). **Do not
  introduce that runtime thunk into production** without a separate
  W^X/CFG/CET/ARM64 lifecycle audit. The first tests avoid it.
- WTL4MinGW's *root* LICENSE declares MIT, but its bundled
  \`wtl/include/atlapp.h\` and \`atlport/include/atldef.h\` still
  carry Common Public License 1.0 / Microsoft / WINX notices. The
  effective licensing and redistribution terms require a legal/source
  provenance review **before vendor, redistribution or product use**.
  Its upstream README explicitly warns about incomplete testing and
  x86-only support. Do not treat the top-level MIT file as superseding
  these file-level terms.
- The source pin is for reproducibility, not a vulnerability or
  compatibility endorsement. No Win32 UI product is released by this
  POC. If a legacy WTL header fails under llvm-mingw Clang, preserve
  the compiler diagnostic and the exact source line; do not patch
  upstream behavior or pass with \`-w -fpermissive\` as a workaround.
