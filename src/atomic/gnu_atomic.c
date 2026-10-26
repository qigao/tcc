#include "poc/atomic32.h"
#if !defined(__GNUC__) && !defined(__clang__)
#error "This backend requires GCC/Clang __atomic builtins. No fallback."
#endif
_Static_assert(sizeof(poc_atomic32) == sizeof(uint32_t), "atomic32 storage must be 4 bytes");
_Static_assert(_Alignof(poc_atomic32) >= 4, "atomic32 storage must be aligned");
uint32_t poc_atomic32_load(const poc_atomic32 *p) { return __atomic_load_n(&p->value, __ATOMIC_SEQ_CST); }
void poc_atomic32_store(poc_atomic32 *p, uint32_t v) { __atomic_store_n(&p->value, v, __ATOMIC_SEQ_CST); }
uint32_t poc_atomic32_exchange(poc_atomic32 *p, uint32_t v) { return __atomic_exchange_n(&p->value, v, __ATOMIC_SEQ_CST); }
uint32_t poc_atomic32_fetch_add(poc_atomic32 *p, uint32_t v) { return __atomic_fetch_add(&p->value, v, __ATOMIC_SEQ_CST); }
int poc_atomic32_compare_exchange(poc_atomic32 *p, uint32_t *e, uint32_t d) {
    return __atomic_compare_exchange_n(&p->value, e, d, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
