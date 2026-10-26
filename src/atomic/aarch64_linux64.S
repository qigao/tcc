/* Linux AArch64 ELF ABI; 8-byte aligned 64-bit atomic slots only. */
#if !defined(__aarch64__) || defined(_WIN32)
#error "aarch64_linux64.S requires native Linux AArch64"
#endif
.text
.p2align 2
.global poc_atomic64_load
.type poc_atomic64_load,%function
poc_atomic64_load:
    ldar x0, [x0]
    ret
.size poc_atomic64_load,.-poc_atomic64_load
.p2align 2
.global poc_atomic64_store
.type poc_atomic64_store,%function
poc_atomic64_store:
    stlr x1, [x0]
    ret
.size poc_atomic64_store,.-poc_atomic64_store
.p2align 2
.global poc_atomic64_exchange
.type poc_atomic64_exchange,%function
poc_atomic64_exchange:
    mov x3, x0
1:
    ldaxr x0, [x3]
    stlxr w4, x1, [x3]
    cbnz w4, 1b
    ret
.size poc_atomic64_exchange,.-poc_atomic64_exchange
.p2align 2
.global poc_atomic64_fetch_add
.type poc_atomic64_fetch_add,%function
poc_atomic64_fetch_add:
    mov x3, x0
1:
    ldaxr x0, [x3]
    add x4, x0, x1
    stlxr w5, x4, [x3]
    cbnz w5, 1b
    ret
.size poc_atomic64_fetch_add,.-poc_atomic64_fetch_add
.p2align 2
.global poc_atomic64_compare_exchange
.type poc_atomic64_compare_exchange,%function
poc_atomic64_compare_exchange:
    ldr x4, [x1]
1:
    ldaxr x5, [x0]
    cmp x5, x4
    b.ne 2f
    stlxr w6, x2, [x0]
    cbnz w6, 1b
    mov w0, #1
    ret
2:
    clrex
    str x5, [x1]
    mov w0, #0
    ret
.size poc_atomic64_compare_exchange,.-poc_atomic64_compare_exchange
.section .note.GNU-stack,"",%progbits
