/* Linux AArch64, natural 8/16-bit atomic instructions; no widened CAS.
 * Acquire/release loads/stores and acquire/release exclusive RMW. */
#if !defined(__aarch64__) || defined(_WIN32)
#error "Requires native Linux AArch64 ELF"
#endif
.text
.p2align 2
.global poc_atomic8_load
.type poc_atomic8_load,%function
poc_atomic8_load:
    ldarb w0,[x0]
    ret
.size poc_atomic8_load,.-poc_atomic8_load
.p2align 2
.global poc_atomic8_store
.type poc_atomic8_store,%function
poc_atomic8_store:
    stlrb w1,[x0]
    ret
.size poc_atomic8_store,.-poc_atomic8_store
.p2align 2
.global poc_atomic8_exchange
.type poc_atomic8_exchange,%function
poc_atomic8_exchange:
    mov x2,x0
1:
    ldaxrb w0,[x2]
    stlxrb w3,w1,[x2]
    cbnz w3,1b
    ret
.size poc_atomic8_exchange,.-poc_atomic8_exchange
.p2align 2
.global poc_atomic8_fetch_add
.type poc_atomic8_fetch_add,%function
poc_atomic8_fetch_add:
    mov x2,x0
1:
    ldaxrb w0,[x2]
    add w3,w0,w1
    stlxrb w4,w3,[x2]
    cbnz w4,1b
    ret
.size poc_atomic8_fetch_add,.-poc_atomic8_fetch_add
.p2align 2
.global poc_atomic8_compare_exchange
.type poc_atomic8_compare_exchange,%function
poc_atomic8_compare_exchange:
    ldrb w4,[x1]
1:
    ldaxrb w5,[x0]
    cmp w5,w4
    b.ne 2f
    stlxrb w6,w2,[x0]
    cbnz w6,1b
    mov w0,#1
    ret
2:
    clrex
    strb w5,[x1]
    mov w0,#0
    ret
.size poc_atomic8_compare_exchange,.-poc_atomic8_compare_exchange
.p2align 2
.global poc_atomic16_load
.type poc_atomic16_load,%function
poc_atomic16_load:
    ldarh w0,[x0]
    ret
.size poc_atomic16_load,.-poc_atomic16_load
.p2align 2
.global poc_atomic16_store
.type poc_atomic16_store,%function
poc_atomic16_store:
    stlrh w1,[x0]
    ret
.size poc_atomic16_store,.-poc_atomic16_store
.p2align 2
.global poc_atomic16_exchange
.type poc_atomic16_exchange,%function
poc_atomic16_exchange:
    mov x2,x0
1:
    ldaxrh w0,[x2]
    stlxrh w3,w1,[x2]
    cbnz w3,1b
    ret
.size poc_atomic16_exchange,.-poc_atomic16_exchange
.p2align 2
.global poc_atomic16_fetch_add
.type poc_atomic16_fetch_add,%function
poc_atomic16_fetch_add:
    mov x2,x0
1:
    ldaxrh w0,[x2]
    add w3,w0,w1
    stlxrh w4,w3,[x2]
    cbnz w4,1b
    ret
.size poc_atomic16_fetch_add,.-poc_atomic16_fetch_add
.p2align 2
.global poc_atomic16_compare_exchange
.type poc_atomic16_compare_exchange,%function
poc_atomic16_compare_exchange:
    ldrh w4,[x1]
1:
    ldaxrh w5,[x0]
    cmp w5,w4
    b.ne 2f
    stlxrh w6,w2,[x0]
    cbnz w6,1b
    mov w0,#1
    ret
2:
    clrex
    strh w5,[x1]
    mov w0,#0
    ret
.size poc_atomic16_compare_exchange,.-poc_atomic16_compare_exchange
.section .note.GNU-stack,"",%progbits
