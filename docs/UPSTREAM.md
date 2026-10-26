# Pinned experiment inputs (not vendored)

| Project | Pin | Purpose / warning |
| --- | --- | --- |
| `TinyCC/tinycc` | `e23dabed154a8495f3ac1ff3cc9d04fa1c207893` | Linux native compiler source; not a complete C11 atomic compiler |
| `mstorsjo/llvm-mingw` | release `20261020`, UCRT x86_64 ZIP, SHA256 `fd5462b7f5d93b2ff981fcd3f9c7e427e47f3203c64ea91c27bcc444eebd74cd` | Windows Clang/LLD, real execution required |
| [kkhaike/openatl](https://github.com/kkhaike/openatl) | `7b587f3e10f3f8d93a01522033de80007368bad5` | C++17 header-only clean-room ATL subset, MIT; isolated llvm-mingw x64 test |
| [waiting4love/WTL4MinGW](https://github.com/waiting4love/WTL4MinGW) | `47dc3c0ff0e44beb8674b6f59533a31c3e08f2a5` | Legacy WTL8/Winx ATL port, x86 only; root MIT but individual headers carry CPL 1.0/Microsoft/Winx notices |
| `katahiromz/RATL` | `2e2efa182d7c6ee9037454d23780991fb8543084` | C++ ReactOS ATL, LGPL-2.1-or-later |
| `DosX-dev/TinyCC-builder` | release `2026-10-22`, ZIP SHA256 `19990b71f401068306ad699aa80af4afaf7f1a7a0110e791022b99da4cbd92e0` | Windows TinyCC/headers distribution; use *non-UPX* asset |
| `DosX-dev/obfus.h` | `52c971a8270e3dd27f3c4fba9f464150b63bf021` | MIT with compiled-distribution exception; opt-in smoke test |
| `eshard/obfuscator-llvm` | `ad26aa21b90cc5c5118d32114b674d1455f54168` | LLVM plugin; Illinois/NCSA-style license; plugin must match host LLVM version |

The GitHub Actions setup pins binary archives by SHA256 and source repositories by immutable commit. These pins are evidence inputs, not a blanket security endorsement. `obfus.h` is obtained from a commit-pinned raw GitHub URL during the **required** Windows semantic test, and retains its own license upstream. No binaries are checked in. `llvm-mingw`'s CRT selection is UCRT and the POC does not mix static objects from incompatible C++ runtimes; only C exports cross module boundaries.

The LLVM 14 host pass qualification applies the audited local compatibility patch `patches/llvm14-demote-phi.patch` to the pinned upstream checkout at build time. This does not claim upstream LLVM 14 native compatibility or alter the upstream repository. The matching host LLVM version and pass output are verified in [the LLVM obfuscation CI](../.github/workflows/llvm-obfuscator-poc.yml).

The pinned TinyCC-builder archive supplies three distinct Windows driver
binaries: `tcc.exe` for x64, `i386-win32-tcc.exe` for Windows x86
native execution, and `arm64-win32-tcc.exe` to produce ARM64 PE
artifacts on the x64 runner. Those artifacts are executed separately on
the native GitHub-hosted `windows-11-vs2026-arm` image after SHA-256
and PE-machine verification. The native ARM64 work does not rely on
Windows-on-ARM x64 emulation for the tested executable processes.

OpenATL and WTL4MinGW are fetched as pinned independent source checkouts by Windows CI. Their overlapping atlbase.h and atlwin.h headers and message-map runtime must **never** share one DLL, include search path or C++ ABI with RATL. Review WTL4MinGW's file-level Common Public License 1.0 / Microsoft notices before redistribution; top-level MIT does not necessarily override them. OpenATL CWndProcThunk uses PAGE_EXECUTE_READWRITE executable memory and is explicitly outside the initial production security gate.
