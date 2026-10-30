# ARM64 obfus.h feasibility — instruction-level POC (not a port)

**Status:** isolated source-only experiment for `qigao/tcc`. Nothing in this folder is a replacement for `DosX-dev/obfus.h`; no production-facing headers or TinyCC binaries are modified. Never silently disable protection when unsupported.

## Evidence and scope

- `DosX-dev/TinyCC-builder` offers `arm64-win32-tcc.exe`; it **compile-tests** ARM64 but does not claim ARM64 runtime for the `obfus.h` integration.
- The upstream `DosX-dev/obfus.h` header has 23 numbered sections and x86/x64-specific instruction strings (e.g. EAX/ESP, encoded `0xE8`/`0xE9` branches, stack proxy frames) and Windows x64 `.pdata` decoys. Replacing its architecture preprocessor branches is **not** enough.
- Pinned TinyCC `mob` commit `e23dabed154a8495f3ac1ff3cc9d04fa1c207893` includes `arm64-asm.c`, whose assembler supports GCC-style extended asm, `eor`, `add`, `sub`, `b`, conditional branches, `%w` registers and constraints. These are suitable seams for safe ARM64-specific lowering.

This POC includes 32-bit modulo arithmetic, two seed-selected reversible instruction sequences, **A64-computed equality predicates**, and build-time encoded string bytes reconstructed into caller-owned memory. The next gate adds **64-bit borrowed object-pointer token encode/decode** (without ownership transfer or dereferencing an encoded token) and C-baseline differential tests covering switch, break/continue, multiple returns, side effects and wrapping unsigned values. Both seed variants must produce identical semantics while producing different binaries; a wrong seed/fixture pair must fail compilation. No compiler flags may select a silent no-op fallback.

**Explicitly unsupported:** complete upstream `obfus.h` macros and automatic keyword rewriting; native x86 call/return/stack proxies; `HIDE_STRING(str)` source compatibility; VM dispatcher virtualization; anti-debugging, trap instructions, fake sections, Windows unwind metadata, CFG, BTI/PAC, signed binaries, and ARM32. The small test is **not** a measure of resistance to reverse engineering. Seed values are public layout selectors, not cryptographic keys. Never use this demo to protect real secrets.

## Native acceptance, not just cross-compile

Run on **actual AArch64 hardware** with GCC, Clang, or pinned TinyCC:

```sh
for seed in 0x12345678 0x12345679; do
  mkdir -p "build/$seed"
  python3 experiments/obfus-arm64/generate_fixture.py \
    --seed "$seed" --output "build/$seed/obfh_arm64_fixture.h"
  tcc -D"OBFH_SEED=${seed}u" -I"build/$seed" \
    experiments/obfus-arm64/smoke.c -o "build/$seed/smoke"
  "build/$seed/smoke"
done
```

A dedicated GitHub Actions workflow verifies native Linux ARM64 semantics with GCC/Clang/TinyCC and real Windows ARM64 native execution of TinyCC-generated PE images. Windows x64 cross-builds are **not** counted as native runtime. The Windows toolchain is installed via the immutable `qigao/vcpkg-cache/.github/actions/setup-tinycc-builder@29317e280bc398ef8b574d2017b4851bfdad3894` with `build-type: toolchain-only`, `target: arm64` and `verify-smoke: true`. That action verifies the `2026-10-29` compiler archive SHA-256 `d1b20a7a85f572201eb00f5e69361fd73f1bbb53ddfb89b49170f33ad2e778d7` and supplies `steps.tcc.outputs.cc`. A successful cross-compile smoke is **not** ARM64 native runtime or `obfus.h` ARM64 Release qualification. The product CI additionally verifies PE machine `0xAA64`, the exact artifact SHA-256 and native execution on actual Windows ARM64. The action's source PR #244 stays Draft.

### Evidence ledger

- POC HEAD `196a9fc4ac9a7145c61c7b7931c3118f7369645f`: [CI #40999773996](https://github.com/qigao/tcc/actions/runs/40999773996) passed 5/5, including Linux native GCC, Clang and TinyCC plus genuine Windows ARM64 10x per seed. These proofs apply to that HEAD only; new pointer/branch changes require new exact-HEAD acceptance.
- Setup action HEAD `29317e280bc398ef8b574d2017b4851bfdad3894`: [CI #40999679537](https://github.com/qigao/vcpkg-cache/actions/runs/40999679537) passed 7/7 for the candidate compiler installer; its Release claims remain explicitly denied.

## Next architectural gates

1. Define `obfus.h` architecture dispatch at its existing section boundaries, preserving unchanged x86/x64 behavior; separate portable entropy/string/VM semantics from ISA-specific code. Upstream remains unmodified until a qualified patch exists.
2. Implement ARM64 stack-/branch-proxy prototypes with genuine A64 encodings and compiler clobber constraints. No SP/LR modifications, no writable-executable sections, no executable junk or skipped-byte gadgets in the first pass; require unwinder correctness before changing frame layout.
3. Prove Windows ARM64 unwind/exception/CFG and BTI/PAC compatibility when relevant, and build *real* differential tests against a no-obfuscation baseline. Reject unsupported options with compile-time errors instead of pretending success.
4. Only then qualify an opt-in TCC `obfus.h` ARM64 distribution; ARM32 and non-Windows deployment require separate work.

Upstream sources: [obfus.h](https://github.com/DosX-dev/obfus.h) · [TinyCC-builder](https://github.com/DosX-dev/TinyCC-builder) · [TinyCC ARM64 asm](https://github.com/TinyCC/tinycc/blob/mob/arm64-asm.c).
