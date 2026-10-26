/* All executable atomic operations live inside the llvm-mingw DLL.
 * Only fixed-width C ABI types cross this library boundary. */
#include "poc/atomic64.h"
#include "poc/atomic_small.h"
#ifdef _WIN32
#define POC_EXPORT __declspec(dllexport)
#define POC_CALL __cdecl
#else
#error "This export module is Windows-only"
#endif
POC_EXPORT unsigned POC_CALL poc_win64_abi_version(void) { return 2u; }
POC_EXPORT unsigned POC_CALL poc_win64_storage_bytes(void) {
    return (unsigned)sizeof(poc_atomic64);
}
POC_EXPORT uint64_t POC_CALL poc_win64_load(const poc_atomic64 *p) {
    return poc_atomic64_load(p);
}
POC_EXPORT void POC_CALL poc_win64_store(poc_atomic64 *p, uint64_t value) {
    poc_atomic64_store(p, value);
}
POC_EXPORT uint64_t POC_CALL poc_win64_exchange(poc_atomic64 *p, uint64_t value) {
    return poc_atomic64_exchange(p, value);
}
POC_EXPORT uint64_t POC_CALL poc_win64_fetch_add(poc_atomic64 *p, uint64_t delta) {
    return poc_atomic64_fetch_add(p, delta);
}
POC_EXPORT int POC_CALL poc_win64_compare_exchange(poc_atomic64 *p,
                                                   uint64_t *expected,
                                                   uint64_t desired) {
    return poc_atomic64_compare_exchange(p, expected, desired);
}

/* Version 2: fixed-width subword dispatcher. Keep all native atomic
 * operations on the llvm-mingw side; TinyCC only handles the strict C ABI.
 * The slot points to a separate 8/16-bit object, not C11 _Atomic storage.
 * Width is bytes; op: 1 load, 2 store, 3 exchange, 4 fetch-add, 5 CAS.
 * CAS: value is expected in/out; return 1 success or 0 failure.
 * Other operations return 0 and update value for load/RMW.
 * Return -1 for invalid requests; never perform a plain-volatile fallback. */
POC_EXPORT int POC_CALL poc_win_small_dispatch(void *slot, unsigned width,
                                                unsigned op, unsigned *value,
                                                unsigned operand) {
    if (!slot || !value || op < 1u || op > 5u) return -1;
    if (width == 1u) {
        volatile uint8_t *p = (volatile uint8_t *)slot;
        uint8_t expected = (uint8_t)*value;
        switch (op) {
          case 1u: *value = __atomic_load_n(p, __ATOMIC_SEQ_CST); return 0;
          case 2u: __atomic_store_n(p, (uint8_t)operand, __ATOMIC_SEQ_CST); return 0;
          case 3u: *value = __atomic_exchange_n(p, (uint8_t)operand, __ATOMIC_SEQ_CST); return 0;
          case 4u: *value = __atomic_fetch_add(p, (uint8_t)operand, __ATOMIC_SEQ_CST); return 0;
          case 5u: {
              int ok = __atomic_compare_exchange_n(p, &expected, (uint8_t)operand, 0,
                         __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
              *value = expected;
              return ok;
          }
        }
    }
    if (width == 2u && ((uintptr_t)slot & 1u) == 0u) {
        volatile uint16_t *p = (volatile uint16_t *)slot;
        uint16_t expected = (uint16_t)*value;
        switch (op) {
          case 1u: *value = __atomic_load_n(p, __ATOMIC_SEQ_CST); return 0;
          case 2u: __atomic_store_n(p, (uint16_t)operand, __ATOMIC_SEQ_CST); return 0;
          case 3u: *value = __atomic_exchange_n(p, (uint16_t)operand, __ATOMIC_SEQ_CST); return 0;
          case 4u: *value = __atomic_fetch_add(p, (uint16_t)operand, __ATOMIC_SEQ_CST); return 0;
          case 5u: {
              int ok = __atomic_compare_exchange_n(p, &expected, (uint16_t)operand, 0,
                         __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
              *value = expected;
              return ok;
          }
        }
    }
    return -1;
}
