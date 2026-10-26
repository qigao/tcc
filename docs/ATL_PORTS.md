# Windows llvm-mingw ATL/WTL header interoperability

This is an independent Windows compiler compatibility POC, not a Salts
production GUI, COM component or ABI. CI clones exact immutable Git
commits; none of these third-party headers or binaries are vendored.

| Upstream | Pin | Scope |
| --- | --- | --- |
| [kkhaike/openatl](https://github.com/kkhaike/openatl) | `7b587f3e10f3f8d93a01522033de80007368bad5` | Modern, C++17, header-only, MIT root license; Windows llvm-mingw x64 and native ARM64 required |
| [waiting4love/WTL4MinGW](https://github.com/waiting4love/WTL4MinGW) | `47dc3c0ff0e44beb8674b6f59533a31c3e08f2a5` | Historical WTL8 and Winx ATL port; **x86 only** per README; Windows i686 required native runtime |
| [katahiromz/RATL](https://github.com/katahiromz/RATL) | `2e2efa182d7c6ee9037454d23780991fb8543084` | Existing LGPL ReactOS ATL independent check; do not mix include trees |

## Compilation and ABI

- OpenATL x64/ARM64 gets only `tools/openatl/include`; WTL4MinGW x86
  gets only `tools/wtl4mingw/atlport/include`,
  `tools/wtl4mingw/stdext/include` and
  `tools/wtl4mingw/wtl/include`. These are separate DLL builds
  and must not share header search order, C++ objects, allocation or
  static link objects with each other or RATL.
- Both DLLs are built by the pinned llvm-mingw C++ compiler; the actual
  Windows TinyCC x86/x64/ARM64 driver compiles a C caller using only
  `LoadLibraryA`, versioned integer C exports and `FreeLibrary`.
  No exceptions, `BSTR`, `HWND`, C++ RTTI or object ownership cross
  the compiler boundary.
- OpenATL test allocates/releases `CComBSTR`, uses a `CSimpleArray<int>`
  and checks Win32 `CHandle` ownership. WTL4MinGW test exercises
  `CAppModule`, `CMessageLoop`, `CIdleHandler` and a hidden
  Win32 `STATIC` window wrapped by ATL::CWindow.
- Fail closed when the compiler, ABI symbols or runtime test fails;
  do not silently switch from Clang to MSVC or MinGW GCC.

## Important limitations and license review

- OpenATL is clean-room MIT according to its root LICENSE and README.
  Its current `CWndProcThunk` implementation allocates writable and
  executable Win32 memory (`PAGE_EXECUTE_READWRITE`). **Do not
  introduce that runtime thunk into production** without a separate
  W^X/CFG/CET/ARM64 lifecycle audit. The first tests avoid it.
- WTL4MinGW's *root* LICENSE declares MIT, but its bundled
  `wtl/include/atlapp.h` and `atlport/include/atldef.h` still
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
  upstream behavior or pass with `-w -fpermissive` as a workaround.

## Exact-head CI milestones

The first OpenATL x64 test successfully built a native C++17 DLL with
`-DUNICODE -D_UNICODE` and executed its TinyCC x64 C ABI caller
five times, so this path is promoted to a **required** CI step.

The WTL4MinGW i686 header build progressed after injecting the C++
`<algorithm>` header, qualifying the ambiguous GUID helper and
excluding the optional outdated `atlctrls.h` API. Its subsequent
link exposed an unresolved `__mingw_uuidof<_CoClassATLVer30>()`
symbol. The pinned-source patch substitutes a local explicit GUID
with the exact bytes specified by the upstream `WINX_DEFINE_CLSID`;
this avoids assuming unsupported MinGW `__uuidof` template
instantiation. The old WTL8 port is now a required CI check after its DLL loaded
and executed five times under Windows TinyCC x86.

## Verified Windows i686 WTL8 C ABI

The pinned `WTL4MinGW` i686 llvm-mingw C++ DLL now builds and
passes five real TinyCC x86 dynamic C-ABI client runs, including
`CAppModule`/message-loop/idle registration and hidden HWND
creation and cleanup. The compatibility patch supplies only an
explicit `GUID_ATLVer30` constant (with the exact source UUID)
in place of an unresolved MinGW `__mingw_uuidof` template symbol;
no GUID semantics are altered. The legacy x86 step is now
**required**, though it remains prohibited from redistribution
before licensing review.

## OpenATL Windows ARM64

A pinned OpenATL C++17 native ARM64 DLL is linked with llvm-mingw;
a native ARM64 TinyCC C client loads it by versioned exports. Both
PE machine types and a SHA-256 manifest are verified on the
real Windows ARM64 runner, with five runtime invocations.
No C++ type, COM pointer or HWND crosses the C ABI. This is the
modern port's explicit ARM64 expansion; it does not qualify
OpenATL's writable/executable `CWndProcThunk`.

## Selection and local reproduction

For a new Windows x64/ARM64 C++ integration prototype, use OpenATL
behind a fixed-width C ABI. WTL4MinGW is a separate legacy x86
compatibility check, **not** a replacement for OpenATL or an excuse to
link the two ATL implementations together. Keep the older RATL test
until consumer behavior is compared deliberately.

On a Windows runner with pinned llvm-mingw and TinyCC installed:

```powershell
git clone https://github.com/kkhaike/openatl.git tools/openatl
git -C tools/openatl checkout --detach 7b587f3e10f3f8d93a01522033de80007368bad5
git clone https://github.com/waiting4love/WTL4MinGW.git tools/wtl4mingw
git -C tools/wtl4mingw checkout --detach 47dc3c0ff0e44beb8674b6f59533a31c3e08f2a5
git -C tools/wtl4mingw apply patches/wtl4mingw-clang-headers.patch
# See scripts/windows-atl-ports.ps1 for required toolchain arguments.
```

The wrapper tests do not instantiate OpenATL's `CWndProcThunk` or
WTL8's full resource-based GUI application. More complete COM
`QueryInterface`, message-map dispatch, window teardown and license
compliance remain Issue #4 gates. The pinned compatibility patch
modifies only CI's temporary checkout, not an upstream Git repository.
