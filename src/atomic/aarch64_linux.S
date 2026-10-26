/* Linux AArch64 ABI, 32-bit aligned independent atomic storage ONLY.
 * ARMv8 acquire/release instructions and exclusive RMW pairs. This is not
 * a replacement for generic C11 _Atomic ABI or unaligned access. */
#if !defined(__aarch64__) || defined(_WIN32)
#error "Requires Linux AArch64 ELF ABI"
#endif
.text
.p2align 2
.global poc_atomic32_load
.type poc_atomic32_load, %function
poc_atomic32_load:
    ldar w0, [x0]
    ret
.size poc_atomic32_load, .-poc_atomic32_load

.p2align 2
.global poc_atomic32_store
.type poc_atomic32_store, %function
poc_atomic32_store:
    stlr w1, [x0]
    ret
.size poc_atomic32_store, .-poc_atomic32_store

.p2align 2
.global poc_atomic32_exchange
.type poc_atomic32_exchange, %function
poc_atomic32_exchange:
    mov x2, x0
1:
    ldaxr w0, [x2]
    stlxr w3, w1, [x2]
    cbnz w3, 1b
    ret
.size poc_atomic32_exchange, .-poc_atomic32_exchange

.p2align 2
.global poc_atomic32_fetch_add
.type poc_atomic32_fetch_add, %function
poc_atomic32_fetch_add:
    mov x2, x0
1:
    ldaxr w0, [x2]
    add w3, w0, w1
    stlxr w4, w3, [x2]
    cbnz w4, 1b
    ret
.size poc_atomic32_fetch_add, .-poc_atomic32_fetch_add

.p2align 2
.global poc_atomic32_compare_exchange
.type poc_atomic32_compare_exchange, %function
poc_atomic32_compare_exchange:
    ldr w4, [x1]
1:
    ldaxr w5, [x0]
    cmp w5, w4
    b.ne 2f
    stlxr w6, w2, [x0]
    cbnz w6, 1b
    mov w0, #1
    ret
2:
    clrex
    str w5, [x1]
    mov w0, #0
    ret
.size poc_atomic32_compare_exchange, .-poc_atomic32_compare_exchange
.section .note.GNU-stack,"",%progbits
