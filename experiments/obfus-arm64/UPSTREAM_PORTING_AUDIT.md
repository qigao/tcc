# Upstream obfus.h → AArch64 porting inventory (read-only audit)

Baseline: [`DosX-dev/obfus.h/include/obfus.h`](https://github.com/DosX-dev/obfus.h/blob/main/include/obfus.h), Git blob SHA `4c06fbc06f907afb3989e2851bed7583cfd0b6b0`, **11,868 lines / 23 numbered regions** at inspection. This document does not vendor, fork, or modify upstream code. Line ranges and risk labels refer to this precise blob, not to future moving `main`.

Initial toolchain: TinyCC `mob` commit `e23dabed154a8495f3ac1ff3cc9d04fa1c207893` (`arm64-asm.c` supports GNU extended asm with constraints and A64 operations). Windows cross-compiler is provided by [`setup-tinycc-builder`](https://github.com/qigao/vcpkg-cache/pull/244) pinned to `29317e280bc398ef8b574d2017b4851bfdad3894`. Its ability to build ARM64 PE **does not** confer obfus.h ARM64 API compatibility.

## Source-region classification

| Upstream region (start line) | Existing contract | ARM64 port decision |
|---|---|---|
| 01 (59) | Compiler and public-disabled/editor paths | Explicitly gate AArch64 TinyCC capability and unsupported options; do not substitute editor or disabled-mode passthrough for protection |
| 02 (306) | Platform section placement | Define A64 PE section/layout rules; preserve x86/x64 section names and behavior unchanged |
| 03 (416) | Compile-time entropy, string and return transport | Reuse unsigned arithmetic where valid; implement typed 64-bit pointer/aggregate return transport using A64 instructions without changing ownership |
| 04 (589) | Retained data / instruction padding | Redesign ISA-specific padding; A64 instructions must be 4-byte aligned and genuinely non-executed data must remain non-executable |
| 05 (820) | Native decoys with PE unwind metadata | **BLOCKED:** x64 `.pdata/.xdata` and epilogue encodings cannot be copied; require Windows ARM64 unwind, exception and CFG-compatible generation |
| 06 (1165) | Skipped payloads / BREAK_STACK_CFLOW | **BLOCKED:** x86 opcode bytes, ModRM, branch displacement and register clobbers require genuine A64 equivalents; no x86 byte gadgets |
| 07 (1943) | STACK_PROXY_FUNCTIONS | **BLOCKED:** native stack-frame tricks require full SP/LR, 16-byte alignment, unwind and lifecycle proof |
| 08 (3046) | Junk primitives and scalar/string transport | Separate pure scalar identities from x86 asm; A64 branch and value transport must preserve flags and clobbers |
| 09 (3218) | CRT cache and VM value transport | **Input `OBFH_VM_VALUE` transport now has isolated source-equivalent byte codec** (8/16-byte `long double`, nonce, floating tag, four xorshift states, two byte arithmetic transforms, per-byte permutation) with independent ABI golden receipts; CRT cache / DLL lifetime remain untouched, original register encoding and VM transport wiring remain open |
| 10 (3359) | CFLOW selectors / permutations / graphs | Preserve conditions and randomization semantics, implement new ARM64 branch patterns with explicit byte width/relocations |
| 11 (4151) | VM register, flag, decode/execute kernel | An **integrated single-owner original-format interpreter** wires packed opcode decode, nonce operand byte transport, four encoded `long double` registers, flags/control and native A64 bitwise ALU. Differential tests now cover **all eight** original `OBFH_V_PROGRAM_0..7` shapes and true runtime PC/opcode trace, including CONTROL-flag and nested conditional return paths; **explicit static const site emission is implemented; automatic caller-side emission, physical context ordering, API/stack/unwind ABI and Release gate remain open** |
| 12 (4856) | Inline string/memory kernels | Preserve byte/UTF-8, buffer capacity, aliasing and return semantics; reject missing original macros instead of silent no-ops |
| 13 (6872) | Automatic C keyword interception | **BLOCKED:** validate nested if/else, loops, switch, case, breaks and returns; do not equate an explicit branch helper to automatic interception |
| 14 (7086) | Public VM interface / program templates | Original eight source program shapes now have a **manually named, per-call-site `static const` packed encoder** and independent byte-differential tests; six direct bitwise and three explicit branch `VM_*` signatures retain prior isolated semantics. **Automatic `OBFH_VM_EXEC`/public `VM_*` call-site macro replacement and  physical seed-selected context layout remain open**, as do keyword rewriting and other API features |
| 15 (7318) | Console/module bootstrap / PE exports | Reuse Windows API where available; prove Windows ARM64 loader/import table and pointer-width ABI |
| 16 (7528) | Loader proxy | Preserve concurrent initialization, handle ownership, module unload and failure behavior |
| 17 (7597) | Anti-debug probes | **BLOCKED:** x86 DR registers and `CONTEXT` fields are not ARM64; no dummy anti-debug markers or bypass |
| 18 (7859) | CRT resolution and formatted-output adapters | Prove Windows ARM64 CRT/UCRT calling conventions, variadic handling and result ownership |
| 19 (8029) | CRT symbol builders and aliases | Requalify symbol names and exact Windows ARM64 import behavior; no assumed x86 decorated names |
| 20 (8372) | CRT argument/result transport | Requalify pointer, aggregate and floating result ABI; no guessed register or stack slots |
| 21 (8521) | Console/Windows API aliases | Audit callbacks, `WINAPI`, typedef width, cross-DLL lifetime and error propagation |
| 22 (11647) | Math aliases / operand transport | Preserve floating-point classifications and NaN/sign behavior with typed differential tests |
| 23 (11833) | Library marker / diagnostics | Preserve clear compile-time failure for unsupported architecture/features; never auto-accept an unimplemented option |

## First accepted experimental subset

- ARM64 instruction-level A64 `ADD/SUB/EOR/ROR` with register-only constraints; no handwritten SP/LR changes.
- 32-bit modular arithmetic and explicit A64-computed equality branch predicate (not full automatic keyword interception).
- 64-bit borrowed pointer encode/decode through integer tokens, including null/stack/static/array/heap; encoded token must never be dereferenced; no CRT ownership transfer.
- Seed-dependent compiled fixture, no-demo-plaintext check, independent C control-flow differential and negative compilation for unsupported original `OBFH_*` toggles, mismatched/out-of-range seeds.
- New opt-in `obfh_arm64_vm_bits.h` exposes `VM_AND/OR/XOR/NOT/SHL/SHR` using actual A64 ISA (not upstream VM virtualization). `vm_bits.c` validates return types, one-time argument evaluation, 32-bit shift masking, boundary values, 4,000 seeded value pairs and nested operations; native Linux/Windows CI required for the new exact HEAD.
- Opt-in `obfh_arm64_vm_branches.h` exposes explicit `VM_IF`, `VM_ELSE_IF`, `VM_ELSE` preserving `!!` single-evaluation and closest-else binding; `vm_branches.c` differentially tests NaN, infinity, pointer/64-bit truth, recursion, goto, break/continue, short-circuit, and nested chains. No automatic keyword interception.
- [Accepted 5/5 at POC HEAD `907a4a28`](https://github.com/qigao/tcc/actions/runs/41002282084). New tests require a new exact-HEAD run before updating evidence.

- New opt-in `obfh_arm64_vm32_program.h` decodes and executes **four immutable encoded words** per source site with actual A64 instructions and strict program/operation validation. `vm_program.c` has successful-decode receipts, negative malformed bytecode and transactional error tests plus 49 boundary and 4,000 pseudo-random semantic inputs. This is a narrow VM with new encoding, **not** upstream's full instruction-program format.

- Added `obfh_arm64_vm32_flow.h` with conditional and unconditional bytecode jumps, A64 TEST/DEC and 1–128 instruction budget. Full pre-validation and transactional output/step receipts, including loops that exhaust their budget. `vm_flow.c` checks 4,096 branch data pairs, 41 precise budget levels, bad targets, key/word corruption, uninitialized flag/register and NULL arguments. Shared `obfh_arm64_vm32_codec.h` is the **only** POC instruction-encoding definition. **Not** upstream opcode packing or full VM equivalence.

- Separate `obfh_arm64_upstream_codec.h` ports **exact** upstream opcode/word/field/layout transforms without adding a new VM execution engine. `vm_upstream_codec.c` independently reproduces the original macro expressions and checks 10,368 vectors across all 1,296 mode combinations against fixed FNV64 `b19831d024873da0`. This test validates byte identity, not key authentication or upstream `Obfh_VirtualMachine` parity. No C/ISA fallback or legacy x86 changes.

- New `obfh_arm64_upstream_vm_bits.h` decodes and **executes the original packed WORD format and opcode numbers** for the strict 4-word unsigned-bitwise subset (real A64 ALU). `vm_upstream_exec.c` exhausts 1,296 profiles × 6 operations × 7 values = 54,432 semantic executions with fail-closed malformed-program and budget tests; it does **not** support upstream encoded long-double register storage, VM program template variations, flags or automatic keyword interception. Keep independent from the prior VM32 POC and do not substitute it as a fallback.

- Original `OBFH_VM_VALUE` input transport is now isolated in `obfh_arm64_upstream_value.h`: byte-preserving object representation and floating/nonce metadata, two arithmetic byte modes, four xorshift maps and ABI-aware 8/16-byte golden fixtures (`vm_upstream_value.c`). It does **not** yet wire original register state or control flags into a production VM. Runtime data corruption is not authenticated; invalid inputs fail without output mutation.

- Source-equivalent four-register storage in `obfh_arm64_upstream_registers.h` maintains the original seed/key/step-derived per-register mask, 3 byte modes, 2 byte traversals, encoded COPY/SWAP and reversible flag/control words. Control `CADD/CXOR/CROL` have bounded signed immediate and ready-bit semantics. [vm_upstream_registers.c](vm_upstream_registers.c) checks 48 state profiles, per-ABI fixed golden receipts, uninitialized guards and transactional rejects. This is a **semantic storage layer**, not the original physical seed-permuted context struct, upstream VM arithmetic or an alternate fallback engine.

- Integrated `obfh_arm64_upstream_runtime.h` and `vm_upstream_runtime.c` implement original-format decoded control-flow, numeric and flags, reusing the **single owner** for the existing upstream codec/register/operand state. All original `OBFH_V_PROGRAM_0..7` instruction patterns are reconstructed from the source definitions and compared against native C results and exact PC/opcode paths. Compile-time immutable expression-site emission and the full public source API/PE ABI remain gated.

- `obfh_arm64_upstream_static.h` emits eight **original-format immutable instruction templates** via explicit named `static const` declaration sites. `vm_upstream_static.c` compares every word with the single source-shaped test reference, confirms read-only-by-contract byte persistence, checks expression one-time evaluation and file-scope type checks, and rejects a key/variant mismatch at compilation. **Automatic per-expression `OBFH_VM_EXEC`, original `__COUNTER__` key derivation, PE section/relocation proof and public VM macros are still open.**

## Admission after this POC

1. **W1:** Define portable core vs ISA-specific lowers in a **separate experimental port**; binary-diff and regression test unchanged original x86/x64 headers before any upstream patch.
2. **W2:** Genuine A64 branch/return/VM helpers with build-seed variation, typed results, real instruction disassembly and negative fail-closed tests.
3. **W3:** Windows ARM64 stack/unwind/CFG and BTI/PAC when applicable, including exception, callback, reload, teardown, and RW→RX safety; never insert a second/unverified fallback.
4. **W4:** Full `HIDE_STRING`, `VM_*`, `BREAK_STACK_CFLOW`, `STACK_PROXY_FUNCTIONS`, C keyword interception and API/CRT wrappers with original semantic comparisons and performance budgets.
5. **W5:** Full native Linux/Windows ARM64 and released downstream consumer qualification; separate ARM32 feasibility. Only then discuss Release admission in `vcpkg-cache`.

**Status: POC only.** No full obfus.h API port, Release qualification, product/compiler switch, anti-debug, fake signature support or claim that seed-based masking is cryptographic protection.
