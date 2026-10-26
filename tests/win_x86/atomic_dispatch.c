/* Build only as an llvm-mingw i686 DLL. No TinyCC object-file mixing.
 * ABI v1: fixed-width uint32_t values, cdecl, exact-width atomic operations.
 * 8/16-bit storage is never accessed using a larger CAS word. */
#include <stdint.h>
#include <stddef.h>
#include "poc/atomic_small.h"
#include "poc/win_x86_atomic.h"
#if !defined(_WIN32) || !defined(__i386__)
#error "Requires 32-bit Windows target"
#endif
_Static_assert(sizeof(poc_atomic8) == 1, "8-bit slot has invalid layout");
_Static_assert(sizeof(poc_atomic16) == 2, "16-bit slot has invalid layout");
_Static_assert(sizeof(poc_atomic_ptr32) == 4, "Pointer32 slot has invalid layout");
_Static_assert(sizeof(poc_atomic64_x86) == 8, "Atomic64 storage is not eight bytes");
_Static_assert(_Alignof(poc_atomic64_x86) >= 8, "Atomic64 requires eight-byte alignment");

__declspec(dllexport) unsigned __cdecl poc_x86_atomic_version(void) {
    return 2u;
}
__declspec(dllexport) unsigned __cdecl poc_x86_atomic_layout(void) {
    return (unsigned)sizeof(poc_atomic8) |
           ((unsigned)sizeof(poc_atomic16) << 8) |
           ((unsigned)sizeof(poc_atomic_ptr32) << 16) |
           ((unsigned)sizeof(poc_atomic64_x86) << 24);
}
/* Operations: 1 load, 2 store, 3 exchange, 4 fetch_add, 5 CAS.
 * On CAS return 1 if equal, 0 if not; observed is always the prior value.
 * For all other operations return 0 and put prior value in observed when
 * relevant. -1 means invalid input; it never modifies caller-owned storage. */
#define DISPATCH_CASE(type) do { \
    volatile type *p = (volatile type *)slot; \
    type expected = (type)*observed; \
    switch (operation) { \
    case 1: *observed = __atomic_load_n(p, __ATOMIC_SEQ_CST); return 0; \
    case 2: __atomic_store_n(p, (type)desired, __ATOMIC_SEQ_CST); return 0; \
    case 3: *observed = __atomic_exchange_n(p, (type)desired, __ATOMIC_SEQ_CST); return 0; \
    case 4: *observed = __atomic_fetch_add(p, (type)desired, __ATOMIC_SEQ_CST); return 0; \
    case 5: { \
        int success = __atomic_compare_exchange_n(p, &expected, (type)desired, \
                              0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
        *observed = expected; \
        return success; \
    } \
    } \
} while (0)
__declspec(dllexport) int __cdecl poc_x86_atomic_dispatch(
    void *slot, unsigned width, unsigned operation, unsigned *observed,
    unsigned desired) {
    if (!slot || !observed || operation < 1u || operation > 5u ||
        (width != 1u && width != 2u && width != 4u)) return -1;
    if (width == 1u) { DISPATCH_CASE(uint8_t); }
    if (width == 2u) {
        if (((uintptr_t)slot & 1u) != 0) return -1;
        DISPATCH_CASE(uint16_t);
    }
    if (((uintptr_t)slot & 3u) != 0) return -1;
    DISPATCH_CASE(uint32_t);
    return -1;
}
#undef DISPATCH_CASE

/* Separate uint64 ABI: x86 cdecl transfers six 32-bit words/pointers
 * across the compiler boundary. A uint64_t is NEVER passed or returned
 * directly between TinyCC and llvm-mingw. The DLL alone handles the
 * aligned 64-bit cmpxchg8b-compatible atomic storage. */
__declspec(dllexport) int __cdecl poc_x86_atomic64_dispatch(
    void *slot, unsigned operation, unsigned *low, unsigned *high,
    unsigned desired_low, unsigned desired_high) {
    volatile uint64_t *p;
    uint64_t operand, expected, previous;
    if (!slot || !low || !high || ((uintptr_t)slot & 7u) != 0u ||
        operation < 1u || operation > 5u) return -1;
    p = (volatile uint64_t *)slot;
    operand = ((uint64_t)desired_high << 32) | desired_low;
    expected = ((uint64_t)(*high) << 32) | *low;
    if (operation == 1u) {
        previous = __atomic_load_n(p, __ATOMIC_SEQ_CST);
    } else if (operation == 2u) {
        __atomic_store_n(p, operand, __ATOMIC_SEQ_CST);
        return 0;
    } else if (operation == 3u) {
        previous = __atomic_exchange_n(p, operand, __ATOMIC_SEQ_CST);
    } else if (operation == 4u) {
        previous = __atomic_fetch_add(p, operand, __ATOMIC_SEQ_CST);
    } else {
        int success = __atomic_compare_exchange_n(
            p, &expected, operand, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        *low = (unsigned)expected;
        *high = (unsigned)(expected >> 32);
        return success;
    }
    *low = (unsigned)previous;
    *high = (unsigned)(previous >> 32);
    return 0;
}
