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
| 09 (3218) | CRT cache and VM value transport | Preserve locking/initialization/ownership; audit compiler atomics, pointer-width transports and DLL lifetime |
| 10 (3359) | CFLOW selectors / permutations / graphs | Preserve conditions and randomization semantics, implement new ARM64 branch patterns with explicit byte width/relocations |
| 11 (4151) | VM register, flag, decode/execute kernel | Both fixed four-word bitwise and new 4–16-word bounded `TEST/JFLAG/JUMP/DEC` A64 VM32 share one seed/site codec; malformed bytecode and execution budget fail closed; **upstream packed format, float/long double and complete VM remain open** |
| 12 (4856) | Inline string/memory kernels | Preserve byte/UTF-8, buffer capacity, aliasing and return semantics; reject missing original macros instead of silent no-ops |
| 13 (6872) | Automatic C keyword interception | **BLOCKED:** validate nested if/else, loops, switch, case, breaks and returns; do not equate an explicit branch helper to automatic interception |
| 14 (7086) | Public VM interface / program templates | Six bitwise `VM_*` source signatures and three explicit `VM_IF/ELSE_IF/ELSE` macros have isolated direct-A64 value transports and semantic tests; upstream encoded VM dispatcher, automatic rewriting and remaining APIs remain **blocked** |
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

## Admission after this POC

1. **W1:** Define portable core vs ISA-specific lowers in a **separate experimental port**; binary-diff and regression test unchanged original x86/x64 headers before any upstream patch.
2. **W2:** Genuine A64 branch/return/VM helpers with build-seed variation, typed results, real instruction disassembly and negative fail-closed tests.
3. **W3:** Windows ARM64 stack/unwind/CFG and BTI/PAC when applicable, including exception, callback, reload, teardown, and RW→RX safety; never insert a second/unverified fallback.
4. **W4:** Full `HIDE_STRING`, `VM_*`, `BREAK_STACK_CFLOW`, `STACK_PROXY_FUNCTIONS`, C keyword interception and API/CRT wrappers with original semantic comparisons and performance budgets.
5. **W5:** Full native Linux/Windows ARM64 and released downstream consumer qualification; separate ARM32 feasibility. Only then discuss Release admission in `vcpkg-cache`.

**Status: POC only.** No full obfus.h API port, Release qualification, product/compiler switch, anti-debug, fake signature support or claim that seed-based masking is cryptographic protection.
