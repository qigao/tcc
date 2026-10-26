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

__declspec(dllexport) unsigned __cdecl poc_x86_atomic_version(void) {
    return 1u;
}
__declspec(dllexport) unsigned __cdecl poc_x86_atomic_layout(void) {
    return (unsigned)sizeof(poc_atomic8) |
           ((unsigned)sizeof(poc_atomic16) << 8) |
           ((unsigned)sizeof(poc_atomic_ptr32) << 16);
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
