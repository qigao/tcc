/* True Windows i686 TinyCC 64-bit Atomic test through a versioned C ABI
 * with ONLY uint32_t words crossing the DLL. No 64-bit ABI assumption. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "poc/win_x86_atomic.h"
#include "poc/threads.h"
#if !defined(__i386__) || !defined(_WIN32)
#error "Windows x86 only"
#endif
enum { WORKERS = 4, LOOPS = 20000, PUBLISH = 1500 };
static poc_atomic64_x86 counter =
    POC_ATOMIC64_X86_INIT(UINT64_C(0x00000001ffffff00));
static poc_atomic64_x86 flag = POC_ATOMIC64_X86_INIT(0);
static unsigned payload;

static int increment(void *arg) {
    int i;
    (void)arg;
    for (i = 0; i < LOOPS; ++i)
        (void)poc_atomic64_x86_fetch_add(&counter, UINT64_C(1));
    return 19;
}
static int producer(void *arg) {
    unsigned i;
    (void)arg;
    for (i = 1; i <= PUBLISH; ++i) {
        while (poc_atomic64_x86_load(&flag) != 0u) Sleep(0);
        payload = i ^ 0xa3b7c21du;
        poc_atomic64_x86_store(&flag, (uint64_t)i);
        while (poc_atomic64_x86_load(&flag) != 0u) Sleep(0);
    }
    return 20;
}
int main(void) {
    const uint64_t base = UINT64_C(0x00000001ffffff00);
    poc_thread workers[WORKERS] = {{0}}, publisher = POC_THREAD_INIT;
    uint64_t expected;
    int i, result, errors = 0;
    if (sizeof(poc_atomic64_x86) != 8u ||
        ((uintptr_t)&counter & 7u) != 0u ||
        ((uintptr_t)&flag & 7u) != 0u) return 1;
    if (poc_x86_atomic_backend_init("poc_x86_atomic.dll")) return 2;
    if (poc_atomic64_x86_load(&counter) != base) return 3;
    expected = base + 1u;
    if (poc_atomic64_x86_compare_exchange(&counter, &expected, 9u) ||
        expected != base) return 4;
    if (!poc_atomic64_x86_compare_exchange(&counter, &expected, base + 3u) ||
        expected != base) return 5;
    if (poc_atomic64_x86_exchange(&counter, base) != base + 3u) return 6;
    if (poc_atomic64_x86_fetch_add(&counter, UINT64_C(0x200000001)) != base ||
        poc_atomic64_x86_load(&counter) != base + UINT64_C(0x200000001))
        return 7;
    poc_atomic64_x86_store(&counter, base);
    for (i = 0; i < WORKERS; ++i)
        if (poc_thread_create(&workers[i], increment, NULL)) return 8;
    for (i = 0; i < WORKERS; ++i)
        if (poc_thread_join(&workers[i], &result) || result != 19) return 9;
    if (poc_atomic64_x86_load(&counter) != base + WORKERS * LOOPS) return 10;

    if (poc_thread_create(&publisher, producer, NULL)) return 11;
    for (i = 1; i <= PUBLISH; ++i) {
        while (poc_atomic64_x86_load(&flag) != (uint64_t)i) Sleep(0);
        if (payload != ((unsigned)i ^ 0xa3b7c21du)) ++errors;
        poc_atomic64_x86_store(&flag, 0u);
    }
    if (poc_thread_join(&publisher, &result) || result != 20 || errors)
        return 12;
    if (poc_x86_atomic_backend_shutdown() ||
        GetModuleHandleA("poc_x86_atomic.dll") != NULL) return 13;
    puts("win-x86-tcc: atomic64 high bits/CAS/carry/threads/publication PASS");
    return 0;
}
