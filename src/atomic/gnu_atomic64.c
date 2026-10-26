#include "poc/atomic64.h"
#if !defined(__GNUC__) && !defined(__clang__)
#error "GCC/Clang __atomic is required. Never fall back to volatile."
#endif
_Static_assert(sizeof(poc_atomic64) == 8, "64-bit atomic storage is exactly 8 bytes");
_Static_assert(_Alignof(poc_atomic64) >= 8, "64-bit atomic storage must align to 8");
uint64_t poc_atomic64_load(const poc_atomic64 *p) {
    return __atomic_load_n(&p->value, __ATOMIC_SEQ_CST);
}
void poc_atomic64_store(poc_atomic64 *p, uint64_t v) {
    __atomic_store_n(&p->value, v, __ATOMIC_SEQ_CST);
}
uint64_t poc_atomic64_exchange(poc_atomic64 *p, uint64_t v) {
    return __atomic_exchange_n(&p->value, v, __ATOMIC_SEQ_CST);
}
uint64_t poc_atomic64_fetch_add(poc_atomic64 *p, uint64_t d) {
    return __atomic_fetch_add(&p->value, d, __ATOMIC_SEQ_CST);
}
int poc_atomic64_compare_exchange(poc_atomic64 *p, uint64_t *expected, uint64_t desired) {
    return __atomic_compare_exchange_n(&p->value, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
