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

## Real Win32 hidden GUI runtime and W^X qualification (2026-10-26)

Source and portable cross-compilation are not a substitute for Windows
GUI runtime. A separate **non-interactive** `tests/wtl10/gui_runtime.cpp`
test now executes Clang++-built Windows GUI code on the actual Windows
x64 and Windows ARM64 hosts, five repetitions per architecture:

- Direct `CWndProcThunk` calls verify the correct hidden `this` pointer
  and target address; each thunk page must be exactly
  `PAGE_EXECUTE_READ` with no writable/executable permissions.
- Real `CWindowImpl` hidden windows dispatch their WTL message maps,
  receive `WM_APP` and `WM_NCDESTROY`, and invoke `OnFinalMessage`
  before the object owning the executable thunk is destroyed.
- Real modeless `CDialogImpl` objects are created from a **compiled
  Windows .rc** resource, verify a string table/control, dispatch
  messages, and explicitly destroy/clear HWND ownership.
- The Linux and macOS hosts independently cross-compile the same
  native-runtime C++/resources for Windows x64 and ARM64, but **do not
  execute** Windows programs. The full upstream `wtltest` SDI GUI still
  only compiles/links with Windows resources; no automated claim about
  interactive full-SDI behavior is made.

Pinned upstream OpenATL remains untouched. An exact-SHA, fail-closed
**test-only patch** (`scripts/openatl-wx-hardening.py`) is applied to
each temporary checkout and rejects any upstream code drift. The patch
changes the thunk transition from RWX to **RW then RX**, checks
`VirtualProtect` and `FlushInstructionCache` results, and fixes an
ARM64 `LDR x16` literal offset (the procedure pointer is at code byte
20, not byte 16). The initial window/dialog thunk initialization also
fails closed instead of invoking a missing thunk.

Evidence at exact head `e6e7b9c0269836e7a463ead5a44a0b6dc331459a`:
[Native Windows WTL10 #40318989038](https://github.com/qigao/tcc/actions/runs/40318989038)
**2/2 PASS**, including true native Windows ARM64;
[Linux/macOS WTL10 #40318988652](https://github.com/qigao/tcc/actions/runs/40318988652)
**2/2 PASS**. Both preserve per-target PE and resource artifacts.

**Still not production qualified:** dynamic CFG/CET target
registration/mitigations; concurrent `WM_NCDESTROY`/late callback and
cancel/unload races; injected `VirtualAlloc`/`VirtualProtect` failures;
real interactive full SDI GUI and accessibility; complete COM
registration/`QueryInterface`; license/distribution compliance.
In particular this temporary patch is **not** an upstream OpenATL fix,
and a passing hidden GUI test does not qualify the production GUI.


This is a standalone compiler experiment. PR #1 remains Draft;
no Salts compiler/ABI/production switch or release is authorized.
