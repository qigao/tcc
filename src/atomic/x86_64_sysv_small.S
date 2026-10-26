/* Linux x86-64 System V ABI, native subword operations; no 32-bit CAS
 * touching neighboring bytes. Natural alignment required. */
#if !defined(__x86_64__) || defined(_WIN32)
#error "x86_64_sysv_small.S requires Linux x86-64 System V"
#endif
.text
.p2align 4
.globl poc_atomic8_load
.type poc_atomic8_load,@function
poc_atomic8_load:
    movzbl (%rdi), %eax
    ret
.size poc_atomic8_load,.-poc_atomic8_load
.p2align 4
.globl poc_atomic8_store
.type poc_atomic8_store,@function
poc_atomic8_store:
    xchgb %sil, (%rdi)
    ret
.size poc_atomic8_store,.-poc_atomic8_store
.p2align 4
.globl poc_atomic8_exchange
.type poc_atomic8_exchange,@function
poc_atomic8_exchange:
    movl %esi,%eax
    xchgb %al,(%rdi)
    movzbl %al,%eax
    ret
.size poc_atomic8_exchange,.-poc_atomic8_exchange
.p2align 4
.globl poc_atomic8_fetch_add
.type poc_atomic8_fetch_add,@function
poc_atomic8_fetch_add:
    movl %esi,%eax
    lock xaddb %al,(%rdi)
    movzbl %al,%eax
    ret
.size poc_atomic8_fetch_add,.-poc_atomic8_fetch_add
.p2align 4
.globl poc_atomic8_compare_exchange
.type poc_atomic8_compare_exchange,@function
poc_atomic8_compare_exchange:
    movzbl (%rsi),%eax
    lock cmpxchgb %dl,(%rdi)
    sete %cl
    movzbl %cl,%ecx
    movb %al,(%rsi)
    movl %ecx,%eax
    ret
.size poc_atomic8_compare_exchange,.-poc_atomic8_compare_exchange
.p2align 4
.globl poc_atomic16_load
.type poc_atomic16_load,@function
poc_atomic16_load:
    movzwl (%rdi),%eax
    ret
.size poc_atomic16_load,.-poc_atomic16_load
.p2align 4
.globl poc_atomic16_store
.type poc_atomic16_store,@function
poc_atomic16_store:
    xchgw %si,(%rdi)
    ret
.size poc_atomic16_store,.-poc_atomic16_store
.p2align 4
.globl poc_atomic16_exchange
.type poc_atomic16_exchange,@function
poc_atomic16_exchange:
    movl %esi,%eax
    xchgw %ax,(%rdi)
    movzwl %ax,%eax
    ret
.size poc_atomic16_exchange,.-poc_atomic16_exchange
.p2align 4
.globl poc_atomic16_fetch_add
.type poc_atomic16_fetch_add,@function
poc_atomic16_fetch_add:
    movl %esi,%eax
    lock xaddw %ax,(%rdi)
    movzwl %ax,%eax
    ret
.size poc_atomic16_fetch_add,.-poc_atomic16_fetch_add
.p2align 4
.globl poc_atomic16_compare_exchange
.type poc_atomic16_compare_exchange,@function
poc_atomic16_compare_exchange:
    movzwl (%rsi),%eax
    lock cmpxchgw %dx,(%rdi)
    sete %cl
    movzbl %cl,%ecx
    movw %ax,(%rsi)
    movl %ecx,%eax
    ret
.size poc_atomic16_compare_exchange,.-poc_atomic16_compare_exchange
.section .note.GNU-stack,"",@progbits
