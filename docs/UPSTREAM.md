# Pinned experiment inputs (not vendored)

| Project | Pin | Purpose / warning |
| --- | --- | --- |
| `TinyCC/tinycc` | `e23dabed154a8495f3ac1ff3cc9d04fa1c207893` | Linux native compiler source; not a complete C11 atomic compiler |
| `mstorsjo/llvm-mingw` | release `20261020`, UCRT x86_64 ZIP, SHA256 `fd5462b7f5d93b2ff981fcd3f9c7e427e47f3203c64ea91c27bcc444eebd74cd` | Windows Clang/LLD, real execution required |
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
