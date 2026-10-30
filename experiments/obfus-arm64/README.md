# ARM64 obfus.h feasibility — instruction-level POC (not a port)

**Status:** isolated source-only experiment for `qigao/tcc`. Nothing in this folder is a replacement for `DosX-dev/obfus.h`; no production-facing headers or TinyCC binaries are modified. Never silently disable protection when unsupported.

## Evidence and scope

- `DosX-dev/TinyCC-builder` offers `arm64-win32-tcc.exe`; it **compile-tests** ARM64 but does not claim ARM64 runtime for the `obfus.h` integration.
- The upstream `DosX-dev/obfus.h` header has 23 numbered sections and x86/x64-specific instruction strings (e.g. EAX/ESP, encoded `0xE8`/`0xE9` branches, stack proxy frames) and Windows x64 `.pdata` decoys. Replacing its architecture preprocessor branches is **not** enough.
- Pinned TinyCC `mob` commit `e23dabed154a8495f3ac1ff3cc9d04fa1c207893` includes `arm64-asm.c`, whose assembler supports GCC-style extended asm, `eor`, `add`, `sub`, `b`, conditional branches, `%w` registers and constraints. These are suitable seams for safe ARM64-specific lowering.

This POC includes 32-bit modulo arithmetic, two seed-selected reversible instruction sequences, **A64-computed equality predicates**, and build-time encoded string bytes reconstructed into caller-owned memory. The next gate adds **64-bit borrowed object-pointer token encode/decode** (without ownership transfer or dereferencing an encoded token) and C-baseline differential tests covering switch, break/continue, multiple returns, side effects and wrapping unsigned values. Both seed variants must produce identical semantics while producing different binaries; a wrong seed/fixture pair must fail compilation. No compiler flags may select a silent no-op fallback. Unknown upstream `OBFH_*` feature flags and legacy `NO_*`/`CFLOW_V2`/`VIRT` options now fail compilation rather than being silently ignored; the 32-bit seed range is also enforced.

**Explicitly unsupported:** complete upstream `obfus.h` macros and automatic keyword rewriting; native x86 call/return/stack proxies; `HIDE_STRING(str)` source compatibility; VM dispatcher virtualization; anti-debugging, trap instructions, fake sections, Windows unwind metadata, CFG, BTI/PAC, signed binaries, and ARM32. The small test is **not** a measure of resistance to reverse engineering. Seed values are public layout selectors, not cryptographic keys. Never use this demo to protect real secrets.

## First public source-signature slice: bitwise `VM_*`

`obfh_arm64_vm_bits.h` is an **opt-in** header separate from the original `DosX-dev/obfus.h`. It exposes exactly six source-compatible macro signatures: `VM_AND`, `VM_OR`, `VM_XOR`, `VM_NOT`, `VM_SHL`, `VM_SHR`. They return `unsigned int`, evaluate each operand once, implement 32-bit unsigned bit patterns, and mask variable shift counts modulo 32 to match upstream region 14. Their bodies use genuine AArch64 `AND/ORR/EOR/LSL/LSR` instructions; `VM_AND` also has two seed-selected reversible instruction sequences.

This is **source-level/semantic parity only**. Upstream's `VM_*` macros route via an encoded VM instruction program, whereas this experimental implementation executes ISA operations directly. It must not be described as VM virtualization, full binary obfuscation parity, or a replacement for the original header. All other `VM_*` names, automatic keyword interception and full `HIDE_STRING(str)` remain unported and unexposed.

`vm_bits.c` is a separate native executable that reproduces upstream [`tests/vm.c`](https://github.com/DosX-dev/obfus.h/blob/main/tests/vm.c)'s bitwise type, evaluation-count and shift semantics. It tests 49 boundary-value pairs, 4,000 deterministic pseudo-random pairs, nested calls and side effects. Linux native GCC/Clang/TinyCC and real Windows ARM64 compile and execute both seed variants; Windows checks `0xAA64` and exact SHA-256 before running.
## Explicit `VM_IF` / `VM_ELSE_IF` / `VM_ELSE` source-signature slice

`obfh_arm64_vm_branches.h` separately exports three original explicit branch macro signatures. Boolean normalization uses `!!(condition)` exactly once, followed by A64 equality/value transport. The `VM_ELSE` form deliberately remains `else if (true)` like the upstream region-14 contract so nested dangling-else behavior is not altered.

`vm_branches.c` follows the logic of upstream [`tests/vm_branches.c`](https://github.com/DosX-dev/obfus.h/blob/main/tests/vm_branches.c): it checks short-circuit side effects, nested `if/else`, 64-bit integer and pointer truth, floating NaN/infinity/signed zero, `goto`, `break/continue`, 1,000 randomized branch chains and 32 recursive cases against unprotected C baselines. This is an **explicit macro** port, not automatic rewriting of C's `if` tokens or the original encoded VM program. The header may be opted into independently; it must never silently claim unsupported upstream features.
## First bounded encoded VM program (W2 experiment)

`obfh_arm64_vm32_program.h` adds a **separate, opt-in real instruction stream interpreter**. Each `VM_AND/OR/XOR/NOT/SHL/SHR` source site owns four immutable, seed/site-encoded 32-bit words: `LOAD_A`, `LOAD_B`, selected `ALU`, `RETURN`. The decoder uses genuine A64 `EOR`, then validates all four decoded words and dispatches actual A64 `AND/ORR/EOR/LSL/LSR` operations. Runtime execution returns an explicit four-step receipt. This is not a C arithmetic fallback, and it does not use W+X memory or rewrite frames.

The executor is **fail closed and transactional**: null operands/output, incorrect program length, reserved bits, invalid opcode/register/return, wrong operation, or mismatched key return a named error without changing the caller-owned result/step receipt. The `VM_*` macros cannot return an error, so malformed encoded programs abort rather than silently computing a plain C result. All input expressions are evaluated once and bitwise results retain `unsigned int` with modulo-32 shifts.

`vm_program.c` verifies 49 boundary pairs, 4,000 deterministic pairs, nested public calls, side-effect counts, decode/dispatch step receipts and negative corrupt-word cases. It builds independently from `obfh_arm64_vm_bits.h`; the earlier direct A64 implementation is an **isolated diagnostic comparison**, not an automatic fallback. CI includes both executables so one cannot silently replace the other. The Windows native gate now requires eight PE variants (four fixture groups × two seeds), each executed ten times with `0xAA64` and SHA-256 validation.

**Remaining gaps:** this compact four-word 32-bit bitwise VM uses a *new experimental format*, not upstream's encoded layout, nonces, mixed integer/floating values, jumps, VM program variants or API call interception. No statement of complete upstream `Obfh_VirtualMachine` compatibility, automatic CFLOW, cryptographic protection or qualified Release is warranted. Next gate is a differential port of upstream bytecode shapes and error semantics with exact instruction-count and bounds proof.
## Bounded encoded VM32 control-flow extension (W2)

`obfh_arm64_vm32_codec.h` is now the **single** VM32 seed/site encoder, key and opcode owner shared by the existing fixed four-word bitwise executor and the new control-flow interpreter. The two experimental executors are separately called for differential qualification; neither is an automatic fallback for the other.

`obfh_arm64_vm32_flow.h` adds decoded `TEST`, conditional `JFLAG`, unconditional `JUMP`, and one-register unsigned `DEC` to a compact **new** experimental VM format. It executes decoded words through A64 `EOR/ORR/AND/SUB/...` register instructions. The interpreter admits 4–16 encoded words and a caller-provided instruction budget from 1–128; out-of-range targets, jumps into the two-word input prologue, malformed opcode/register/immediate bits and absent terminal RETURN are rejected before execution. Runtime use of an uninitialized register or flag fails rather than reading guessed state.

Validation first, execution second: **any failure leaves both caller-owned output and executed-step receipt unchanged**, even if a loop has already executed many steps. This is a transactional caller-visible contract, not protection against crashes or a security sandbox. It never writes executable memory, changes SP/LR or runs alternate engines. On success it returns the executed instruction count, not merely the encoded length; backward loops terminate through `max_steps`.

`vm_flow.c` checks 4,096 deterministic conditional branch pairs against C, both branch paths and exact execution counts, count-down loop budgets (including exact/admissible and one-less/rejected), unsigned-zero underflow loops, invalid/misaligned branches, wrong seed, reserved and invalid words, uninitialized register/flag, NULL arguments, and budget overflow. Each negative case checks the sentinel output and step receipt. The original VM32 bitwise and source-macro fixtures remain independent and green-only on the exact PR HEAD. CI builds this added executable with GCC/Clang/TinyCC native AArch64 plus the hash-verified Windows cross-build and genuine Windows ARM64 runner, bringing the matrix to **10 executables (5 program groups × 2 seeds), 10 real native repetitions each**.

This extension is still **not** the upstream `obfus.h` encoded VM format: upstream has a richer opcode permutation, flags and long-double register layout, and distinct branch semantics. Support is confined to the explicitly documented POC instruction subset; the Release gate remains closed.
## Exact upstream packed-word encoder/decoder qualification (region 11/14)

`obfh_arm64_upstream_codec.h` is a **separate, purely 32-bit C11** port of the original obfus.h packed-word encoding/decoding formulas, anchored to upstream Git blob `4c06fbc06f907afb3989e2851bed7583cfd0b6b0` (rather than a moving branch). It includes all **6 opcode transforms**, all **9 word mask/encode transforms**, all **6 D/A/B field permutations**, all **4 packed field layouts**, plus the key-dependent rotation and the 3-round modular inverse for odd opcode multipliers. C11 is sufficient here: upstream encodes 32-bit words arithmetically, without an x86-specific instruction requirement. The existing VM32 encoder remains the only owner of the *separate POC* VM32 format; do not silently conflate or substitute the two wire formats.

`vm_upstream_codec.c` compares every output to an independently expressed copy of upstream `OBFH_V_INSTRUCTION`/`OBFH_V_PACK`/`OBFH_V_ENCODE_RAW` semantics and verifies decode roundtrips. It exhaustively covers 6×9×6×4 = **1,296 mode combinations**, each with eight opcode/register/immediate/PC vectors (**10,368 encoded words**). A separately computed, fixed 64-bit FNV-1a golden digest `b19831d024873da0` protects against comparing two identically incorrect encoders. Invalid pointer, PC, immediate, opcode and register inputs fail without overwriting output.

This is an **exact packed-word codec contract**, not the full original `Obfh_VirtualMachine` implementation. Every bit in the 32-bit instruction is used, so arbitrary bit flips and wrong keys still decode to *some* tuple; the encoding is not authenticated and corrupt programs require independent opcode, operands and execution-flow validation. The upstream 4-register long-double runtime, nonce handling, flags, VM program templates, keyword interception, exception/CFG/unwind and downstream Release qualification remain open. The codec test is an extra native executable for Linux GCC/Clang/TinyCC and Windows TinyCC, raising the Windows native matrix to **12 PE variants × 10 runs** without creating a second CI workflow.
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

Detailed upstream source-region ownership and the split between portable semantics and ISA-specific rewrites are recorded in [UPSTREAM_PORTING_AUDIT.md](UPSTREAM_PORTING_AUDIT.md), based on the exact upstream Git blob SHA.

## Next architectural gates

1. Define `obfus.h` architecture dispatch at its existing section boundaries, preserving unchanged x86/x64 behavior; separate portable entropy/string/VM semantics from ISA-specific code. Upstream remains unmodified until a qualified patch exists.
2. Implement ARM64 stack-/branch-proxy prototypes with genuine A64 encodings and compiler clobber constraints. No SP/LR modifications, no writable-executable sections, no executable junk or skipped-byte gadgets in the first pass; require unwinder correctness before changing frame layout.
3. Prove Windows ARM64 unwind/exception/CFG and BTI/PAC compatibility when relevant, and build *real* differential tests against a no-obfuscation baseline. Reject unsupported options with compile-time errors instead of pretending success.
4. Only then qualify an opt-in TCC `obfus.h` ARM64 distribution; ARM32 and non-Windows deployment require separate work.

Upstream sources: [obfus.h](https://github.com/DosX-dev/obfus.h) · [TinyCC-builder](https://github.com/DosX-dev/TinyCC-builder) · [TinyCC ARM64 asm](https://github.com/TinyCC/tinycc/blob/mob/arm64-asm.c).
