# Compiler POC architecture and admission gates

## Layering

1. **Compiler:** upstream TinyCC for C; llvm-mingw/Clang for C/C++, Windows PE/COFF and resource compilation. No C++ parser or AST is added to TCC.
2. **Platform:** separate POSIX `pthread`/Win32 `CreateThread`, mutex, condition, and TLS destructor backends. Objects stay with their creating compiler and owner, never cross a DLL boundary.
3. **Atomic32 storage:** a **separate** naturally aligned four-byte `poc_atomic32`. Linux x86-64 GAS (`lock xadd`, `cmpxchg`, `xchg`, TSO load); Linux AArch64 acquire/release exclusive instructions (`LDAR/STLR/LDAXR/STLXR`); Windows `Interlocked*`; GCC/Clang `__atomic` baseline. All POC atomic operations are sequentially consistent. `volatile` alone is NOT synchronization.
4. **Consumers:** identical thread+CAS+publication C client for all atomic backends. Windows clients additionally test PE DLL exports, dynamic lookup, and a separate LLVM-mingw-built C++/RATL bridge. Obfuscation is isolated from ABI/concurrency qualification.

This POC ABI is **not** binary compatible with `_Atomic`, `std::atomic` or Salts `SALTS_ATOMIC_TYPE`. No public Salts ABI is modified. Use a C ABI consisting only of fixed-width scalars, caller-provided arrays, explicit error codes, and specified calling conventions. Never pass STL types, C++ exceptions, `FILE*`, allocator-owned buffers, or CRT handles between compilers. The examples have no cross-boundary allocation.

## Admission matrix

| Gate | Linux GCC/Clang | Linux TCC | Windows LLVM | Windows TCC |
| --- | --- | --- | --- | --- |
| real compile/link/run | required | required | required | required before promotion |
| atomic32 RMW, failed CAS expected | required | required | required | required |
| four workers / exact increments | required | required | required | required |
| release/acquire publication | required | required | required | required |
| Win32 wait/event/clock | N/A | N/A | required | required x64 |
| C ABI DLL runtime | N/A | N/A | required | required x64 |
| RATL C++ behind C export | N/A | N/A | required x64 sample | required x64 caller |
| bounded FIFO / close / TLS destructors | required | required | required x64 | required x64 |
| native AArch64 GAS | required native ARM64 | required native ARM64 | N/A | N/A |
| obfus.h semantic parity | N/A | N/A | N/A | required x64 smoke |
| LLVM14 host pass and IR change | required Linux x64 job | N/A | NOT the current llvm-mingw target | N/A |

A **passed compilation** does not establish ABI/layout, memory-order semantics, exception confinement, weak-memory portability, installed SDK correctness, or sanitizer coverage. In particular, native ASM is not automatically observable by TSan; run GCC/Clang source TSan/ASan/UBSan for separately implemented atomics and acquire/release handoffs. For Windows check both compile + **actual execution**; cross-compilation alone is insufficient. On AArch64 use only the separate ELF ABI backend and native ARM64 execution; on Windows ARM64 use Windows intrinsics and native hardware. x86 TSO results cannot substitute for AArch64 memory-model results. The new 32-bit assembly backend does not imply 8/16/64-bit atomics or arbitrary C11 `_Atomic` support.

## Native atomic64 and pointers (bounded)

A separate eight-byte slot implements `load/store/exchange/fetch_add/CAS`; pointer operations reuse that slot **by composition**, never by strict-aliasing conversion. The implementation is deliberately restricted to native x86-64/AArch64 Linux or Windows x64 (and GCC/Clang native 64-bit references). Windows uses a distinct calling convention for `win_x64.S`: slot in RCX, expected in RDX, desired in R8. The Windows TinyCC smoke separately tests **the actual imported COFF object** built by llvm-mingw. No 32-bit x86 object may link or call it. These are independent native POC storage types, *not* generic compiler `_Atomic` compatibility. Caller retains all pointed-to object lifetimes through publication and consumer acknowledgement.

## Thread ownership

The `poc_thread`, `poc_mutex`, `poc_condition` and `poc_tss` types are **private ABI test objects**. No fields may cross compilers or DSOs; callers zero-initialize the storage and keep its address stable. The callback result is published before successful join and read after join. Closing the queue holds its mutex and broadcasts to all waiters. A separate capacity-one fixture deterministically parks an admitted writer on a full queue and an admitted reader on an empty queue, then closes the owner while each is actually waiting, verifies exact fail-closed outcomes, and checks absence of phantom reads/writes. Thread-specific destructor callbacks must run before the owner deletes its key, with Win32 FLS and POSIX pthread implementations independently exercised. Destroying with blocked waiters or racing writers remains invalid; later tests will add explicit busy-error policies rather than silently masking undefined lifetime.

## RATL and Windows C ABI

A C++ DLL compiled by llvm-mingw may instantiate RATL templates internally and export `extern "C" __cdecl int poc_ratl_sum(...)`. The TinyCC consumer resolves that export with `LoadLibraryA` + `GetProcAddress`. Each module creates and destroys its own objects; the C boundary passes fixed-size arrays. On x86 the calling convention and exported name decoration must be checked independently; initial scope is x86-64. RATL is LGPL-2.1-or-later and remains an external pinned checkout; do not vendor without attribution and license review.

## LLVM host-pass qualification

The pinned eshard LLVM plugin is built against the same LLVM 14 headers/libraries as `opt-14`, using `patches/llvm14-demote-phi.patch` to reconcile upstream PHI demotion and FCmpInst call signatures with LLVM 14. The required test uses an explicit new-pass-manager `function(flattening)` pipeline; it verifies that IR is actually changed, verifies transformed IR and compares seven runtime inputs. This is host Linux LLVM14 only; latest LLVM/Clang versions shipped with llvm-mingw require a separately built matching plugin and additional native PE tests.

## Obfuscation acceptance

Baseline and transformed variants must produce identical exit code and stdout over known deterministic inputs. Use fixed toolchain source SHA, header SHA, plugin version, build flags and seed; archive compiler versions and diagnostics. If an LLVM pass does not change the generated representation, semantic parity **does not** prove it ran. No obfuscation is a security boundary; do not conflate it with native thread/atomic conformance. Never enable fake signatures or anti-debugging in CI.

## Next gates

1. Make the x86-64 TinyCC GAS workflow a mandatory job after exact pinned-version runtime evidence.
2. Qualify llvm-mingw C/Win32 on actual Windows runner, including DLL load/unload.
3. Diagnose RATL Clang/MinGW compatibility separately; keep legacy PSEH, resource and COM APIs within C++ DLL.
4. Expand the now-required Windows TinyCC/RATL/obfus.h x64 smoke into x86 calling-convention, ownership, cleanup and deeper COM/ATL tests.
5. Add layout/versioned ABI and native ARM64 ownership/cancellation stress tests before any downstream usage. Do not mark Salts TinyCC-ready based on these PoCs.
