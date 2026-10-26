#if !defined(__x86_64__) || defined(_WIN32)
#error "This assembly requires the Linux x86-64 System V ABI."
#endif
.text
.p2align 4
.globl poc_atomic32_load
.type poc_atomic32_load,@function
poc_atomic32_load:
    movl (%rdi), %eax
    ret
.size poc_atomic32_load,.-poc_atomic32_load
.p2align 4
.globl poc_atomic32_store
.type poc_atomic32_store,@function
poc_atomic32_store:
    xchgl %esi, (%rdi)
    ret
.size poc_atomic32_store,.-poc_atomic32_store
.p2align 4
.globl poc_atomic32_exchange
.type poc_atomic32_exchange,@function
poc_atomic32_exchange:
    movl %esi, %eax
    xchgl %eax, (%rdi)
    ret
.size poc_atomic32_exchange,.-poc_atomic32_exchange
.p2align 4
.globl poc_atomic32_fetch_add
.type poc_atomic32_fetch_add,@function
poc_atomic32_fetch_add:
    movl %esi, %eax
    lock xaddl %eax, (%rdi)
    ret
.size poc_atomic32_fetch_add,.-poc_atomic32_fetch_add
.p2align 4
.globl poc_atomic32_compare_exchange
.type poc_atomic32_compare_exchange,@function
poc_atomic32_compare_exchange:
    movl (%rsi), %eax
    lock cmpxchgl %edx, (%rdi)
    sete %cl
    movzbl %cl, %ecx
    movl %eax, (%rsi)
    movl %ecx, %eax
    ret
.size poc_atomic32_compare_exchange,.-poc_atomic32_compare_exchange
.section .note.GNU-stack,"",@progbits
