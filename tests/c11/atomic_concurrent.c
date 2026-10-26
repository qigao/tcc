/* Native C11 _Atomic with the existing private thread facade.
 * The facade supplies workers only, not atomic layout or ordering.
 */
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include "poc/threads.h"

enum { WORKERS = 4, ADDITIONS = 10000, PUBLICATIONS = 1500 };
static atomic_uint total = ATOMIC_VAR_INIT(0u);
static atomic_ullong wide_total = ATOMIC_VAR_INIT(0ull);
static atomic_uint ready = ATOMIC_VAR_INIT(0u);
static atomic_uint acknowledged = ATOMIC_VAR_INIT(0u);
static unsigned payload;

static int add_worker(void *arg) {
    unsigned i;
    (void)arg;
    for (i = 0; i < ADDITIONS; ++i) {
        (void)atomic_fetch_add_explicit(&total, 1u, memory_order_relaxed);
        (void)atomic_fetch_add_explicit(&wide_total, 1ull, memory_order_relaxed);
    }
    return 17;
}
static int publish_worker(void *arg) {
    unsigned i;
    (void)arg;
    for (i = 1; i <= PUBLICATIONS; ++i) {
        while (atomic_load_explicit(&acknowledged, memory_order_acquire) != i - 1u) { }
        payload = i;
        atomic_store_explicit(&ready, i, memory_order_release);
    }
    return 19;
}
int main(void) {
    poc_thread t[WORKERS + 1] = {{0}};
    unsigned i;
    int result;
    for (i = 0; i < WORKERS; ++i) {
        if (poc_thread_create(&t[i], add_worker, NULL)) return 1;
    }
    for (i = 0; i < WORKERS; ++i) {
        if (poc_thread_join(&t[i], &result) || result != 17) return 2;
    }
    if (atomic_load(&total) != WORKERS * ADDITIONS ||
        atomic_load(&wide_total) != (unsigned long long)WORKERS * ADDITIONS) return 3;

    if (poc_thread_create(&t[WORKERS], publish_worker, NULL)) return 4;
    for (i = 1; i <= PUBLICATIONS; ++i) {
        while (atomic_load_explicit(&ready, memory_order_acquire) != i) { }
        if (payload != i) return 5;
        atomic_store_explicit(&acknowledged, i, memory_order_release);
    }
    if (poc_thread_join(&t[WORKERS], &result) || result != 19) return 6;
    puts("C11 atomic concurrent: relaxed RMW, release/acquire publication PASS");
    return 0;
}
