# TinyCC + llvm-mingw interoperability laboratory

This repository is an **experimental, standalone C ABI / compiler qualification lab**. It does not replace Salts's production C11 atomics or declare TinyCC conforming to C11. The experiments deliberately do not vendor or patch any upstream compiler, Windows SDK, RATL, or obfuscator.

## Scope and status

| Experiment | Required behavior | Current admission |
| --- | --- | --- |
| Linux GCC/Clang + atomic32 | real four-worker RMW/CAS and publish/consume | locally exercised |
| Linux TinyCC + external GAS atomic32 | same API and threaded tests | required x64 CI |
| Windows llvm-mingw | Win32 handles, events, C ABI DLL | required x64 CI |
| Windows TinyCC + `Interlocked` | same atomic32/thread client | real Win x64 runtime qualified |
| TCC C client + llvm-mingw C DLL | `LoadLibrary`/`GetProcAddress`, values/ownership | required Windows x64 CI |
| TCC C client + llvm-mingw C++/RATL DLL | RATL templates and BSTRs stay inside LLVM-built DLL | required Windows x64 sample |
| `obfus.h` on Windows TinyCC | semantic parity to unobfuscated source, 2 seeds | required Windows x64 smoke |
| C11-style thread facade | mutex, condition, join, TLS destructors, bounded queue | compiler matrix qualification |
| Native 64-bit atomic and pointer CAS | Linux GNU/GAS, AArch64 and Windows x64 TCC with platform-specific ASM | required compiler matrix |
| Linux native AArch64 atomic32 | LDAR/STLR, LDAXR/STLXR and real 8-thread workload | native ARM64 CI qualification |
| `obfuscator-llvm` pass | verified IR transformation + seven-input parity under LLVM 14 | required host-LLVM14 workflow; not llvm-mingw |

`RATL` means *ReactOS Active Template Library*, a **Windows C++ ATL** implementation, **not** an atomic primitives library. TinyCC compiles C only: all RATL C++ objects stay behind an exported, version-checked C ABI. Windows and Linux platform backends are distinct and never silently fall back to plain volatile storage.

## Linux quick start

```sh
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DPOC_ATOMIC_BACKEND=gnu
cmake --build build
ctest --test-dir build --output-on-failure

cmake -S . -B build-gas -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_ASM_COMPILER=gcc -DPOC_ATOMIC_BACKEND=gas-x86_64
cmake --build build-gas
ctest --test-dir build-gas --output-on-failure
```

On Linux ARM64 use `-DPOC_ATOMIC_BACKEND=gas-aarch64` with a native AArch64 GAS assembler. Both the new thread contract and atomic tests run on actual ARM64 runners. For TinyCC on native Linux x86-64 set `CMAKE_C_COMPILER` to your pinned `tcc` and `CMAKE_ASM_COMPILER=gcc`, then require `gas-x86_64`. The GAS backend consists of fixed-size 32-bit System V ABI operations; it does **not** pretend to implement generic `_Atomic(T)` or the entire C11 memory model.

## Native 64-bit atomic and pointer CAS

`include/poc/atomic64.h` introduces a **separate, eight-byte-aligned** 64-bit word and an 8-byte pointer slot. The latter converts pointer representations through `uintptr_t` only on 64-bit flat-address platforms. It does **not** reinterpret `_Atomic(T)`, `std::atomic` or a Salts object. On Linux x86-64/AArch64, the TinyCC implementation links platform-specific external GAS instructions; on Windows x64 llvm-mingw links a separate **Microsoft ABI** object into a DLL, and the TinyCC client calls it through a versioned C API. This is deliberate: directly linking the llvm-mingw `.obj` into this TinyCC distribution failed with `unrecognized file type`. GCC/Clang baselines use `__atomic`. No platform silently falls back to `volatile` operations. Cross-compiler layout, lifetime and DSO unloading remain outside this narrow API.

The isolated thread test checks CAS failure writeback, 64-bit arithmetic above 4 GiB, real concurrent increments, pointer exchange/CAS, and release/acquire pointer-message publication. An object published by pointer must stay allocated until all readers release it.

## Thread facade scope

`include/poc/threads.h` is an intentionally limited `pthread`/Win32 adapter, not a drop-in C11 `threads.h` or a stable public ABI. It provides create/join, mutex, condition-variable waits/broadcast, and destructor-backed thread-specific storage. The bounded FIFO test runs four producers, four consumers, 10,000 distinct payloads and a queue capacity of seven, then checks exact-once consumption, closing wakeups, rejected late writes, return values and exactly eight TLS destructor callbacks. Live thread, mutex, condition and TLS objects must never move or cross DLL/compiler boundaries. They must be destroyed or joined only by the owning test.

## Windows i686 C ABI probe

The Windows job separately builds a **real 32-bit PE** DLL/client with llvm-mingw's `i686-w64-mingw32-clang`, runs them on Windows, and checks `cdecl` vs `stdcall` entry points, stack balance, version handshake and caller/DLL struct layout. The x86 test does **not** claim Windows TinyCC x86 compatibility; that requires a distinct native i686 TCC toolchain and tests.

## Windows quick start

Use **llvm-mingw UCRT x86_64** and **TinyCC for Windows**. Configure/build/test the CMake suite with llvm-mingw's `x86_64-w64-mingw32-clang.exe`; run `scripts/windows-interop.ps1` with pinned compiler roots, RATL tree and `obfus.h` header to exercise actual mixed-compiler executables and DLLs. The Windows script compiles TinyCC clients and performs runtime tests; linking C++ objects into TCC is not permitted.

## Obfuscation boundary

- `DosX-dev/obfus.h` is a Windows x86/x64 TinyCC header. The smoke test disables anti-debugging, PDATA decoys, and fake signatures; it checks output parity, not resistance to reverse engineering. **It is not included in production headers or concurrency tests.**
- `eshard/obfuscator-llvm` is an LLVM pass plugin, built for the **host platform** and **matching LLVM version** that loads it. It cannot be loaded into TinyCC; the project's LLVM 14 build example is not automatically compatible with current llvm-mingw Clang. `scripts/llvm-pass-smoke.sh` is opt-in.
- Source is fetched by explicit commit reference; keep upstream attribution/licenses intact rather than copying these projects into this repository.

See [design and evidence gates](docs/DESIGN.md), and [dependency pins](docs/UPSTREAM.md).

## Windows x64 DLL admission and drain

The cross-toolchain DLL binding now has a **bounded 64-lease admission state machine**, with fail-closed READY → DRAIN → UNLOADING → EMPTY transitions. `poc_atomic64_backend_shutdown()` returns `POC_ATOMIC64_BUSY` for in-flight borrowed or ordinary atomic operations. Every operation holds a lease until the native function returns; the loader does not call `FreeLibrary` until all leases are retired. The Windows TinyCC acceptance test exercises capacity 64/65, a genuine parked worker holding a lease, concurrent drain, refusal of new work, token double-return rejection, final unload and quiescent reopen. This remains a *private POC* ABI, not a production DSO API.

## 8-bit and 16-bit native atomic matrix

New private `poc_atomic8`/`poc_atomic16` APIs use exact-size storage and
independent native instructions or compiler builtins. Test coverage includes
overflow, failed-CAS writeback, exchange/add under contention, exact-width
neighbor sentinel integrity and message publication. Windows TinyCC uses a
versioned llvm-mingw DLL dispatcher admitted through the same bounded lease
Owner; it does not pretend to accept unsupported native COFF object inputs.

## Windows 32-bit native TinyCC diagnostic

The pinned TinyCC-builder release provides `i386-win32-tcc.exe`.
A required Windows runner test compiles and **executes** the x86 ABI
`cdecl/stdcall` client, Atomic32 contention, the bounded Threads/TLS
fixture and the shutdown-overlap fixture. The FLS destructor is
explicitly a `WINAPI` trampoline instead of casting a C callback
to the incompatible x86 `stdcall` type. The source remains a private POC. The Windows x86 TinyCC gate is now **required**, with 20 repeated executions of the two lifecycle fixtures; full RATL/COM support on x86 has not been qualified.

## Windows ARM64 cross-compilation (not runtime)

The pinned Windows TinyCC-builder also provides `arm64-win32-tcc.exe`.
On the x64 Windows runner a **required, compile-only** job uses
`aarch64-w64-mingw32-clang.exe` and `arm64-win32-tcc.exe`
to generate Windows PE images. It verifies the PE signature and
`IMAGE_FILE_MACHINE_ARM64` (`0xAA64`) and builds sample Windows API,
Atomic32 and bounded Threads clients. This does **not** execute Windows
ARM64 binaries or qualify native ARM64 Windows thread/atomic behavior.
Only a native Windows ARM64 machine/runner may close that runtime gate.

## Windows i686 TinyCC: exact-width Atomic8/16 and pointer32

The native `i386-win32-tcc.exe` now exercises an llvm-mingw i686
DLL providing 8/16/32-bit `__atomic` primitives through a narrow,
versioned **cdecl** API. TinyCC calls `LoadLibrary/GetProcAddress`
and validates exported ABI version and storage layout. Tests cover
subword overflow, failed-CAS expected writeback, neighboring sentinel
integrity, four-worker contention, and real release/acquire publication.
An independent pointer32 test adds 2,400 producer/consumer handoffs,
bad-ABI rejection, and quiescent DLL unload/reopen.

This does **not** make TinyCC accept llvm-mingw COFF object files.
Unlike the separately qualified Windows x64 DLL Owner, the x86
DLL initialization and unload remain **quiescent-only**.

## Windows i686 DLL bounded admission

The i686 TinyCC-to-llvm-mingw dispatcher now has an independent
**64-lease Interlocked owner**, mirroring the fail-closed admission
semantics previously qualified on x64. A real parked worker retains
its lease while the main thread requests shutdown. The API must return
`BUSY`, reject new borrow/init attempts, preserve the live DLL, and
unload only after that worker returns and joins. Tests also cover
lease-capacity exhaustion, token double return, wrong-ABI rejection,
repeated unload refusal and quiescent reload.

This is still a limited POC. Copying a live token, using a DLL
function pointer outside the owner, or attempting uncontrolled
cross-process unload is not a supported contract.
