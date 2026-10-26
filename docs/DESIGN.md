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

A separate eight-byte slot implements `load/store/exchange/fetch_add/CAS`; pointer operations reuse that slot **by composition**, never by strict-aliasing conversion. The implementation is deliberately restricted to native x86-64/AArch64 Linux or Windows x64 (and GCC/Clang native 64-bit references). Windows uses a distinct calling convention for `win_x64.S`: slot in RCX, expected in RDX, desired in R8. The Windows TinyCC smoke tests a version-checked **DLL C ABI**, because this TinyCC distribution rejects the llvm-mingw COFF object file with `unrecognized file type`. The llvm-mingw-built DLL contains the Windows x64 GAS implementation. The TCC client fails closed on missing ABI symbols/version and may initialize/unload the library only while quiescent. The Windows test also loads a deliberately incompatible DLL, requires rejection and release with no retained module handle, then checks proper unload, duplicate unload rejection and a fresh quiescent reinitialize/release cycle.

The Windows x64 C facade now uses a **single 32-bit Interlocked CAS state word**: EMPTY, READY, DRAIN, MUTATING plus a bounded lease count in units of four (maximum 64). Every exported call claims a lease before dereferencing function pointers; shutdown atomically moves READY to DRAIN, denies new leases and returns explicit BUSY until retained owners have returned. No FreeLibrary occurs with a live lease. A parked real Windows worker retains a lease while main initiates shutdown and verifies BUSY/no new admissions, then signals release; only after join may the final unload succeed. Lease tokens are non-copyable by contract, owner-managed, generation-stamped and fail on repeat return. This does not yet cover uncontrolled callers violating the lease/token discipline, OS module injection, or complete DSO lifecycle across process isolation. No 32-bit x86 object may link or call it. These are independent native POC storage types, *not* generic compiler `_Atomic` compatibility. Caller retains all pointed-to object lifetimes through publication and consumer acknowledgement.

## Thread ownership

The `poc_thread`, `poc_mutex`, `poc_condition` and `poc_tss` types are **private ABI test objects**. No fields may cross compilers or DSOs; callers zero-initialize the storage and keep its address stable. The callback result is published before successful join and read after join. Closing the queue holds its mutex and broadcasts to all waiters. A separate capacity-one fixture deterministically parks an admitted writer on a full queue and an admitted reader on an empty queue, then closes the owner while each is actually waiting, verifies exact fail-closed outcomes, and checks absence of phantom reads/writes. Thread-specific destructor callbacks must run before the owner deletes its key, with Win32 FLS and POSIX pthread implementations independently exercised. Destroying with blocked waiters or racing writers remains invalid; later tests will add explicit busy-error policies rather than silently masking undefined lifetime.

## Separate Windows i686 ABI gate

The LLVM-mingw i686 toolchain builds and executes a 32-bit Windows PE DLL/client. A dedicated caller checks exported `cdecl`/`stdcall` function names, both calling conventions, ESP restoration and a layout handshake for a fixed-width C structure. This test is independent of the 64-bit Atomic backend. It qualifies only llvm-mingw i686, **not** TinyCC's 32-bit assembler/linker or full ATL/COM support.

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

## Native small-width atomics

Separate `poc_atomic8` and `poc_atomic16` primitives use naturally aligned,
exact-width storage. Native GNU/Clang uses real `__atomic`, Linux x86-64
TinyCC uses 8/16-bit `xchg`/`lock xadd`/`cmpxchg`, and Linux ARM64
TinyCC uses `LDARB/LDARH/STLRB/STLRH/LDAXRB/LDAXRH/STLXRB/STLXRH`.
The fixtures require exact subword operations by placing sentinels on both
sides of the storage (no unsafe expanded 32-bit CAS and no alias of Salts
`_Atomic` storage). Windows llvm-mingw uses Clang's native 8/16-bit `__atomic`
for a version-2 DLL `poc_win_small_dispatch` C ABI, and TinyCC calls this
through the existing 64-lease protected DLL facade. Any unknown operation,
version, width or unaligned 16-bit request fails closed. Tests cover rollover
CAS/RMW, 4-worker contention and 8-bit/16-bit acquire/release publication.
The architecture still does not qualify generic `stdatomic.h` semantics.

## Windows x86 FLS callback ABI hardening

The pinned Windows TinyCC package includes `i386-win32-tcc.exe`, so its
32-bit runtime is tested independently rather than inferred from i686 Clang.
The initial diagnostic x86 run revealed a real `0xC0000005` crash during repeated
thread/TLS tests. The original code wrongly cast a `cdecl` user destructor
to Win32 `PFLS_CALLBACK_FUNCTION` (`stdcall` on x86). The replacement
registers a genuine `VOID WINAPI` callback. Each FLS value is held in a
small process-heap wrapper containing the callback and value; the trampoline
releases its wrapper and dispatches to the user callback with its correct
calling convention. Repeated TSS set of the same key reuses the wrapper and
must still deliver exactly one destructor. The patched Windows x86 TinyCC
ABI/Atomic32 tests are now required and both lifecycle fixtures repeat 20 times. No STL, C++ object, or CRT-owned
memory crosses the DLL ABI. Key destruction remains quiescent-only.

## Windows ARM64 cross-compiler gate

The pinned `arm64-win32-tcc.exe` and llvm-mingw AArch64 drivers generate
PE/COFF ARM64 sample executables on an x64 CI host. Machine word `0xAA64`
is independently parsed from the PE header. The gate covers compile and
link of selected Windows API, Atomic32 and thread sources; it does not
execute these binaries. Windows ARM64 native execution and system
interoperability are still explicitly unqualified.

## Windows TinyCC i686 subword and pointer32 qualification

Native TinyCC x86 clients use an llvm-mingw i686 DLL with a fixed-width
`cdecl` dispatcher: width 1, 2 or 4 bytes; load/store/exchange/add/CAS.
All operations are emitted by Clang as genuine `__atomic` primitives
on dedicated storage, never as non-atomic `volatile` RMW. The DLL
returns exact prior values, writes back the expected value on failed CAS,
and rejects misaligned 16/32-bit slots. Export name-decoration fallbacks
and ABI version/size negotiation are explicit. The client owns pointers
and thread lifetimes; no cross-CRT buffers, C++ objects or file handles
are exchanged. The 32-bit pointer CAS/publication test rejects mismatched
ABI DLLs and verifies unload/reopen only after all borrowers have joined.
This i686 proof does not yet implement the x64 concurrent lease drain.

## Windows i686 DLL admission and drain

The TinyCC x86 facade uses the real Win32 InterlockedCompareExchange
full-barrier operation to atomically pair a phase
`EMPTY/READY/DRAIN/MUTATING` and a bounded outstanding-lease count.
Each call obtains a lease **before** dereferencing the i686 llvm-mingw
function pointer and returns it only after that function has returned.
The closing owner transitions to DRAIN in one CAS, refuses additional
calls and returns BUSY while existing leases are retained. Once the
last lease has retired, FreeLibrary may execute. A real worker parks
with a loaned token to deterministically test close-before-return.
The token is generation-stamped and may not be copied or returned
more than once; no process, thread, or DLL field escapes the owner.

## Windows x86 Atomic64 word-pair ABI

The i686 DLL ABI version 2 extends the existing exact-width
8/16/32-bit dispatcher with a separate `poc_x86_atomic64_dispatch`.
The C ABI accepts slot, operation, pointers to low/high 32-bit prior
values and low/high desired values. Only the llvm-mingw side executes
native 64-bit `__atomic` instructions. The caller must hold
8-byte-aligned independent storage; unaligned slots are rejected
without touching adjacent memory. The TinyCC side never passes or
returns a 64-bit integer *across the DLL boundary*, avoiding an
untested compiler-specific lowering. Operations still hold a lease
for the entire native call. This is a **private** ABI, not a
replacement for C11 `stdatomic.h`.
