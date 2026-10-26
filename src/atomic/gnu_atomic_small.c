#include "poc/atomic_small.h"
#if !defined(__clang__) && !defined(__GNUC__)
#error "Only GNU/Clang real __atomic builtins are supported"
#endif
_Static_assert(sizeof(poc_atomic8) == 1, "atomic8 must occupy 1 byte");
_Static_assert(sizeof(poc_atomic16) == 2, "atomic16 must occupy 2 bytes");
_Static_assert(_Alignof(poc_atomic16) >= 2, "atomic16 alignment must be 2");
#define POC_DEFINE_SMALL(W,T) \
T poc_atomic##W##_load(const poc_atomic##W *p) { \
    return __atomic_load_n(&p->value, __ATOMIC_SEQ_CST); \
} \
void poc_atomic##W##_store(poc_atomic##W *p, T v) { \
    __atomic_store_n(&p->value, v, __ATOMIC_SEQ_CST); \
} \
T poc_atomic##W##_exchange(poc_atomic##W *p, T v) { \
    return __atomic_exchange_n(&p->value, v, __ATOMIC_SEQ_CST); \
} \
T poc_atomic##W##_fetch_add(poc_atomic##W *p, T v) { \
    return __atomic_fetch_add(&p->value, v, __ATOMIC_SEQ_CST); \
} \
int poc_atomic##W##_compare_exchange(poc_atomic##W *p, T *expected, T desired) { \
    return __atomic_compare_exchange_n(&p->value, expected, desired, 0, \
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
}
POC_DEFINE_SMALL(8, uint8_t)
POC_DEFINE_SMALL(16, uint16_t)
