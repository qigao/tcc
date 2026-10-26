# Compiler POC architecture and admission gates

## Layering

1. **Compiler:** upstream TinyCC for C; llvm-mingw/Clang for C/C++, Windows PE/COFF and resource compilation. No C++ parser or AST is added to TCC.
2. **Platform:** POSIX `pthread`/Win32 `CreateThread` and events. Backends are concrete and fail compilation if unavailable.
3. **Atomic32 storage:** a **separate** naturally aligned four-byte `poc_atomic32`. Linux x86-64 GAS (`lock xadd`, `cmpxchg`, `xchg`, TSO load); Windows `Interlocked*`; GCC/Clang `__atomic` baseline. All POC atomic operations are sequentially consistent. `volatile` alone is NOT synchronization.
4. **Consumers:** identical thread+CAS+publication C client for all atomic backends. Windows clients additionally test PE DLL exports, dynamic lookup, and a separate LLVM-mingw-built C++/RATL bridge. Obfuscation is isolated from ABI/concurrency qualification.

This POC ABI is **not** binary compatible with `_Atomic`, `std::atomic` or Salts `SALTS_ATOMIC_TYPE`. No public Salts ABI is modified. Use a C ABI consisting only of fixed-width scalars, caller-provided arrays, explicit error codes, and specified calling conventions. Never pass STL types, C++ exceptions, `FILE*`, allocator-owned buffers, or CRT handles between compilers. The examples have no cross-boundary allocation.

## Admission matrix

| Gate | Linux GCC/Clang | Linux TCC | Windows LLVM | Windows TCC |
| --- | --- | --- | --- | --- |
| real compile/link/run | required | required | required | required before promotion |
| atomic32 RMW, failed CAS expected | required | required | required | required |
| four workers / exact increments | required | required | required | required |
| release/acquire publication | required | required | required | required |
| Win32 wait/event/clock | N/A | N/A | required | diagnostic |
| C ABI DLL runtime | N/A | N/A | required | diagnostic |
| RATL C++ behind C export | N/A | N/A | diagnostic | diagnostic caller |
| obfus.h semantic parity | N/A | N/A | N/A | diagnostic |
| LLVM plugin semantic parity | opt-in | N/A | opt-in matching plugin | N/A |

A **passed compilation** does not establish ABI/layout, memory-order semantics, exception confinement, weak-memory portability, installed SDK correctness, or sanitizer coverage. In particular, native ASM is not automatically observable by TSan; run GCC/Clang source TSan/ASan/UBSan for separately implemented atomics and acquire/release handoffs. For Windows check both compile + **actual execution**; cross-compilation alone is insufficient. Before attempting ARM64, add a separate audited AArch64 assembly backend (or Win32 Interlocked where applicable) and run native hardware tests. Do not run x86 GAS on ARM64.

## RATL and Windows C ABI

A C++ DLL compiled by llvm-mingw may instantiate RATL templates internally and export `extern "C" __cdecl int poc_ratl_sum(...)`. The TinyCC consumer resolves that export with `LoadLibraryA` + `GetProcAddress`. Each module creates and destroys its own objects; the C boundary passes fixed-size arrays. On x86 the calling convention and exported name decoration must be checked independently; initial scope is x86-64. RATL is LGPL-2.1-or-later and remains an external pinned checkout; do not vendor without attribution and license review.

## Obfuscation acceptance

Baseline and transformed variants must produce identical exit code and stdout over known deterministic inputs. Use fixed toolchain source SHA, header SHA, plugin version, build flags and seed; archive compiler versions and diagnostics. If an LLVM pass does not change the generated representation, semantic parity **does not** prove it ran. No obfuscation is a security boundary; do not conflate it with native thread/atomic conformance. Never enable fake signatures or anti-debugging in CI.

## Next gates

1. Make the x86-64 TinyCC GAS workflow a mandatory job after exact pinned-version runtime evidence.
2. Qualify llvm-mingw C/Win32 on actual Windows runner, including DLL load/unload.
3. Diagnose RATL Clang/MinGW compatibility separately; keep legacy PSEH, resource and COM APIs within C++ DLL.
4. Qualify a reproducible Windows TinyCC toolchain and compare `obfus.h`-protected/unprotected program semantics.
5. Add layout/versioned ABI and native ARM64 ownership/cancellation stress tests before any downstream usage. Do not mark Salts TinyCC-ready based on these PoCs.
