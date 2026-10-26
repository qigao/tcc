# WTL 10.0 + OpenATL / llvm-mingw (preferred modern path)

## Why a new integration path?

[OpenATL](https://github.com/kkhaike/openatl) explicitly targets
**WTL 10.0** with Clang/MinGW C++17. Its author maintains
[wtltest](https://github.com/kkhaike/wtltest), a resource-based
SDI/window/control demo. That is a stronger modern integration target
than trying to upgrade the WTL8/Winx ATL runtime bundled in
[WTL4MinGW](https://github.com/waiting4love/WTL4MinGW).

Keep all three checks independent:
1. **OpenATL + WTL10** — preferred llvm-mingw Windows C++ application layer.
2. **WTL4MinGW (WTL8)** — legacy x86/x64/ARM64 port regression only.
3. **RATL** — separate ReactOS-derived ATL ABI reference.

The TinyCC bridge is a **C API**. WTL templates and exceptions
compile under llvm-mingw Clang++, not TinyCC.

## Immutable upstream inputs

| Component | Source | Pin |
|---|---|---|
| OpenATL | https://github.com/kkhaike/openatl | 7b587f3e10f3f8d93a01522033de80007368bad5 |
| WTL10 sample | https://github.com/kkhaike/wtltest | 63ba6a992cb3d79d56be66cd283f6615c6646b3d |
| WTL10 10.0.10320 | https://github.com/Win32-WTL/WTL (official release mirror, originally SourceForge) | Mirror commit 065ef5597383c1231bd4892cf8e94331b29bdb97, blob a9f9d84495aac2155b0fdb00fec0e421b0e75c90 (Releases/WTL10_10320_Final.zip) |
| llvm-mingw | Existing pinned toolchain | 20261020 UCRT |

The workflow fetches the ZIP at the pinned mirror commit, verifies
the **full Git blob SHA** (not merely a filename) and extracts it into
a temporary CI directory. No WTL header ZIP is committed to qigao/tcc.
WTL10 uses **MS-PL**; retain that license and copyright notices.
WTL4MinGW WTL8 has different CPL-era historical notices.

## Acceptance contract

- On Windows x64 and ARM64 llvm-mingw: build C++17 DLL with only
  OpenATL headers + extracted WTL10 headers (not WTL4MinGW/RATL).
- Build a real TinyCC C client for the corresponding Windows ABI, load
  a versioned C DLL and test WTL10 CMessageLoop/IdleHandler, native
  EDIT/BUTTON/ListView/TreeView control wrappers, 64-bit item-data
  retention, module apartment init and teardown.
- Native Windows ARM64 **must execute**, not merely cross-compile.
- A separate full WTL10 \`wtltest\` SDI/RC build is a compilation
  target; do not claim GUI/UX runtime from linking alone.
- Exclude unqualified OpenATL \`CWndProcThunk\` RWX executable memory.
  This limited test creates raw USER32 parents and uses WTL wrappers;
  actual WTL10 CWindowImpl thunk hardening is separate.
- Keep PR #1 Draft and do not change Salts production compiler or ABI.

## Completed qualification — October 26, 2026

The **required** Windows x64/ARM64 interoperability workflow
[`wtl10-openatl.yml`](../.github/workflows/wtl10-openatl.yml)
retrieves WTL 10.0.10320 from the immutable release mirror, checks
both its Git blob identity and SHA-256:
`13de5cd7ed41454b8f287e9bcc4525234c6ac8c85905e4d4fe1a0776709fd78f`.

The llvm-mingw C++17 provider, with actual WTL10 message loops and
control wrappers, is exercised by real TinyCC C clients:
**Windows x64 5/5**, **native Windows ARM64 5/5**.
The native ARM64 job verifies CPU type, PE machine and SHA-256
artifact hashes before running.

The separate **required**
[`wtl10-cross-host.yml`](../.github/workflows/wtl10-cross-host.yml)
checks both host toolchain release hashes and uses genuine
**Linux** and **macOS** host processes to cross-compile Windows
x64/ARM64 DLLs with OpenATL and WTL10. It also builds and links the
author's complete WTL10 `wtltest` SDI GUI, including the Win32
resources (`.rc`), for both target architectures.
Verified initial cross-host success: [CI #40315361815](https://github.com/qigao/tcc/actions/runs/40315361815).

The interactive full SDI GUI has **not** been exercised in native
UI automation, and OpenATL's window-procedure thunk can request
PAGE_EXECUTE_READWRITE; do not promote that full message-map
implementation to hardened product use until W^X/CFG/CET/ARM64 and
window callback lifecycle qualification passes. Our required
native runtime test deliberately uses a raw USER32 parent and
WTL10 control wrappers, without invoking that thunk.

WTL 10.0 headers are **MS-PL** licensed and were not vendored;
the full WTL10 ZIP is downloaded for CI only. WTL4MinGW 8.x
continues as a separately pinned, legacy port and requires its
own source-file license review. Do not mix their ATL headers or
C++ runtime objects with OpenATL in one translation unit.
