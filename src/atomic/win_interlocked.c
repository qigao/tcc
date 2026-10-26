#include <windows.h>
#include "poc/atomic32.h"
#if !defined(_WIN32)
#error "Win32 Interlocked backend requires Windows."
#endif
/* Interlocked operations provide a conservative full-barrier contract here. */
uint32_t poc_atomic32_load(const poc_atomic32 *p) {
    return (uint32_t)InterlockedCompareExchange((volatile LONG *)&p->value, 0, 0);
}
void poc_atomic32_store(poc_atomic32 *p, uint32_t v) {
    (void)InterlockedExchange((volatile LONG *)&p->value, (LONG)v);
}
uint32_t poc_atomic32_exchange(poc_atomic32 *p, uint32_t v) {
    return (uint32_t)InterlockedExchange((volatile LONG *)&p->value, (LONG)v);
}
uint32_t poc_atomic32_fetch_add(poc_atomic32 *p, uint32_t delta) {
    return (uint32_t)InterlockedExchangeAdd((volatile LONG *)&p->value, (LONG)delta);
}
int poc_atomic32_compare_exchange(poc_atomic32 *p, uint32_t *expected, uint32_t desired) {
    uint32_t previous = (uint32_t)InterlockedCompareExchange((volatile LONG *)&p->value,
                                                             (LONG)desired, (LONG)*expected);
    if (previous == *expected) return 1;
    *expected = previous;
    return 0;
}
