# Official WTL10 + OpenATL: single Windows WTL path

The only WTL source supported by this POC is official WTL 10.0.10320
with OpenATL. WTL8/WTL4MinGW compatibility code has been deleted;
there is no fallback and no sharing of ATL include trees.
Clang++ owns all C++/COM/WTL objects and exports only a versioned C ABI.
TinyCC remains a C compiler; no Windows GUI/C++ ABI support is claimed.

## Immutable sources

- OpenATL: https://github.com/kkhaike/openatl at 7b587f3e10f3f8d93a01522033de80007368bad5
- WTL10 demo: https://github.com/kkhaike/wtltest at 63ba6a992cb3d79d56be66cd283f6615c6646b3d
- WTL10 ZIP (official release mirror): Win32-WTL/WTL commit
  065ef5597383c1231bd4892cf8e94331b29bdb97, blob
  a9f9d84495aac2155b0fdb00fec0e421b0e75c90, SHA256
  13de5cd7ed41454b8f287e9bcc4525234c6ac8c85905e4d4fe1a0776709fd78f
- llvm-mingw: 20261020 UCRT, SHA256 pinned per Windows/Linux/macOS runner.

WTL10 carries MS-PL notices; sources are fetched temporarily and
checked against immutable pins, not vendored in this project.

## Mandatory evidence and known limits

1. Windows x64 native TinyCC C client loads the Clang++ WTL10 DLL
   five times; validates ABI, idle/message loop, COM ownership,
   EDIT/BUTTON/ListView/TreeView wrappers and 64-bit item data.
2. Windows ARM64 must verify PE machine/hash and execute the same
   ABI client five times on a true ARM64 Windows runner.
3. Genuine Linux and macOS hosts must independently use their pinned
   llvm-mingw distributions to cross-build x64+ARM64 Windows DLLs,
   compile actual upstream WTL10 SDI sources, and link the complete
   SDI example with the resource compiler output for wtltest.rc.
   Each executable must be 64-bit PE for the expected target and
   contain a nonempty .rsrc directory. Both host jobs are blocking.
4. Resource objects, linked .exe files and DLLs are CI artifacts.
   Neither fsyntax-only nor linking a DLL alone meets this gate.

A linked GUI executable is not evidence of interactive GUI operation.
Message-map/GUI interaction, real resource display, window teardown,
CFG/CET and security lifecycle require separately designed execution
tests. OpenATL CWndProcThunk currently uses RWX pages, deliberately
excluded from the limited hidden Win32 wrapper probe. Do not use it
in a production GUI without W^X and platform security qualification.

This is a standalone compiler experiment. PR #1 remains Draft;
no Salts compiler/ABI/production switch or release is authorized.
