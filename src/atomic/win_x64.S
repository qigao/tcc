/* Windows x64 Microsoft ABI: slot RCX, value/expected RDX, desired R8.
 * Deliberately separate from System V GAS. Leaf routines need no unwind
 * metadata because they do not touch RSP or callee-saved registers. */
#if !defined(__x86_64__) || !defined(_WIN32)
#error "win_x64.S requires a Windows x64 target"
#endif
.text
.p2align 4
.globl poc_atomic64_load
poc_atomic64_load:
    movq (%rcx), %rax
    ret
.p2align 4
.globl poc_atomic64_store
poc_atomic64_store:
    xchgq %rdx, (%rcx)
    ret
.p2align 4
.globl poc_atomic64_exchange
poc_atomic64_exchange:
    movq %rdx, %rax
    xchgq %rax, (%rcx)
    ret
.p2align 4
.globl poc_atomic64_fetch_add
poc_atomic64_fetch_add:
    movq %rdx, %rax
    lock xaddq %rax, (%rcx)
    ret
.p2align 4
.globl poc_atomic64_compare_exchange
poc_atomic64_compare_exchange:
    movq (%rdx), %rax
    lock cmpxchgq %r8, (%rcx)
    sete %r9b
    movq %rax, (%rdx)
    movzbl %r9b, %eax
    ret
