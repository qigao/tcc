/* x86-64 Linux System V ABI; 8-byte aligned atomic slots only. */
#if !defined(__x86_64__) || defined(_WIN32)
#error "x86_64_sysv64.S requires Linux x86-64 System V"
#endif
.text
.p2align 4
.globl poc_atomic64_load
.type poc_atomic64_load,@function
poc_atomic64_load:
    movq (%rdi), %rax
    ret
.size poc_atomic64_load,.-poc_atomic64_load
.p2align 4
.globl poc_atomic64_store
.type poc_atomic64_store,@function
poc_atomic64_store:
    xchgq %rsi, (%rdi)
    ret
.size poc_atomic64_store,.-poc_atomic64_store
.p2align 4
.globl poc_atomic64_exchange
.type poc_atomic64_exchange,@function
poc_atomic64_exchange:
    movq %rsi, %rax
    xchgq %rax, (%rdi)
    ret
.size poc_atomic64_exchange,.-poc_atomic64_exchange
.p2align 4
.globl poc_atomic64_fetch_add
.type poc_atomic64_fetch_add,@function
poc_atomic64_fetch_add:
    movq %rsi, %rax
    lock xaddq %rax, (%rdi)
    ret
.size poc_atomic64_fetch_add,.-poc_atomic64_fetch_add
.p2align 4
.globl poc_atomic64_compare_exchange
.type poc_atomic64_compare_exchange,@function
poc_atomic64_compare_exchange:
    movq (%rsi), %rax
    lock cmpxchgq %rdx, (%rdi)
    sete %cl
    movzbl %cl, %ecx
    movq %rax, (%rsi)
    movl %ecx, %eax
    ret
.size poc_atomic64_compare_exchange,.-poc_atomic64_compare_exchange
.section .note.GNU-stack,"",@progbits
