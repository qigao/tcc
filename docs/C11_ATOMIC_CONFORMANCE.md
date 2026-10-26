# C11 atomic compiler conformance (M0)

**Scope:** compiler-native `<stdatomic.h>` with independently compiled source,
not the experimental `poc_atomic*` storage ABI. Tracked by
[Issue #2](https://github.com/qigao/tcc/issues/2); PR #1 remains Draft.

## Required runtime contracts

- `tests/c11/atomic_scalar.c`: native 8/16/32/64-bit atomic storage,
  width-isolated rollover, strong/weak CAS including failure expected
  writeback, RMW, pointer CAS, flag, fences and lock-free query.
- `tests/c11/atomic_concurrent.c`: four-worker relaxed RMW and an
  independent release/acquire publication protocol transferring
  non-atomic payload only after the acquiring load.
- CMake/CTest runs on native GCC, Clang and Linux x86-64/ARM64
  TinyCC. Windows llvm-mingw Clang and macOS Clang run the positive tests.
- Pinned TinyCC on **Windows x64 and macOS x86-64/ARM64** defines
  `__STDC_NO_ATOMICS__` (confirmed by failing native tests on first CI run).
  The explicit `expect_no_atomics` compile/run gate requires that the
  unsupported capability is reported; it is **NOT** a passing C11
  conformance test. The existing independent `poc_atomic*` native POC
  acceptance remains required. If this capability changes, fail the
  negative gate and replace it with real positive compiler tests.
- There must be **no** `-Iinclude` replacement of the toolchain's
  `stdatomic.h`; only `poc/threads.h` is used for worker admission.
  Never reinterpret an `_Atomic(T)` as `poc_atomic*` or a C++ atomic.

## Known gaps (not qualified by M0)

- Full ISO C11 header completeness (including `ATOMIC_*_LOCK_FREE`,
  all typedefs, generic operations, representation and all memory orders).
  Pinned upstream TinyCC header has no `ATOMIC_*_LOCK_FREE` macros.
- Weak-memory litmus/systematic modeling, 32-bit 64-bit atomicity and
  lock-free/alignment truthfulness, compiler miscompile differential
  testing, and native Android ARM64 hardware.
- Cross-compiler `_Atomic` / `mtx_t` binary layout and ABI. A
  versioned fixed-width C ABI is mandatory for any DLL bridge.
- Complete `<threads.h>` interface (M2) and installed consumer SDK (M4).
- GCC/Clang source TSan does not instrument TinyCC-generated atomics or
  the separately assembled GAS backend. Results must remain separate.

Existing `poc_atomic*` tests remain unchanged; M0 does not claim Salts,
CNet, CFlow or any production compiler migration.
