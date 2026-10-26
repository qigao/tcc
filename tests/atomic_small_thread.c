#include "poc/atomic_small.h"
#include "poc/threads.h"
#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#if defined(POC_ATOMIC64_EXTERNAL_DLL)
#include "poc/atomic64.h"
#endif
#else
#include <sched.h>
#endif
enum { WORKERS = 4, ITERATIONS = 20000, PUBLISH = 1600 };
typedef struct cage8 {
    uint8_t before;
    poc_atomic8 counter;
    uint8_t after;
} cage8;
typedef struct cage16 {
    uint16_t before;
    poc_atomic16 counter;
    uint16_t after;
} cage16;
static cage8 slot8 = { 0xa5u, POC_ATOMIC8_INIT(0), 0x5au };
static cage16 slot16 = { 0xa55au, POC_ATOMIC16_INIT(0), 0x5aa5u };
static poc_atomic8 signal8 = POC_ATOMIC8_INIT(0);
static poc_atomic16 signal16 = POC_ATOMIC16_INIT(0);
static int packet;
static void yield_cpu(void) {
#ifdef _WIN32
    Sleep(0);
#else
    sched_yield();
#endif
}
static int increment(void *unused) {
    int i;
    (void)unused;
    for (i = 0; i < ITERATIONS; ++i) {
        (void)poc_atomic8_fetch_add(&slot8.counter, 1u);
        (void)poc_atomic16_fetch_add(&slot16.counter, 1u);
    }
    return 17;
}
static int publish(void *unused) {
    int i;
    (void)unused;
    for (i = 1; i <= PUBLISH; ++i) {
        while (poc_atomic8_load(&signal8) != 0) yield_cpu();
        packet = i;
        poc_atomic8_store(&signal8, 1u);
        while (poc_atomic8_load(&signal8) != 0) yield_cpu();
    }
    for (i = 1; i <= PUBLISH; ++i) {
        while (poc_atomic16_load(&signal16) != 0) yield_cpu();
        packet = i ^ 0x7654;
        poc_atomic16_store(&signal16, (uint16_t)i);
        while (poc_atomic16_load(&signal16) != 0) yield_cpu();
    }
    return 18;
}
int main(void) {
    poc_thread workers[WORKERS] = {{0}}, producer = POC_THREAD_INIT;
    uint8_t expected8;
    uint16_t expected16;
    int i, result, errors = 0;
#if defined(_WIN32) && defined(POC_ATOMIC64_EXTERNAL_DLL)
    if (poc_atomic64_backend_init("poc_atomic64_win.dll")) return 29;
#endif
    if (sizeof(poc_atomic8) != 1 || sizeof(poc_atomic16) != 2 ||
        ((uintptr_t)&slot16.counter & 1u) != 0u) return 1;
    poc_atomic8_store(&slot8.counter, 255u);
    expected8 = 1;
    if (poc_atomic8_compare_exchange(&slot8.counter, &expected8, 9u) ||
        expected8 != 255u) return 2;
    if (!poc_atomic8_compare_exchange(&slot8.counter, &expected8, 5u) ||
        expected8 != 255u) return 3;
    if (poc_atomic8_exchange(&slot8.counter, 255u) != 5u) return 4;
    if (poc_atomic8_fetch_add(&slot8.counter, 2u) != 255u ||
        poc_atomic8_load(&slot8.counter) != 1u) return 5;

    poc_atomic16_store(&slot16.counter, 65535u);
    expected16 = 1;
    if (poc_atomic16_compare_exchange(&slot16.counter, &expected16, 9u) ||
        expected16 != 65535u) return 6;
    if (!poc_atomic16_compare_exchange(&slot16.counter, &expected16, 7u) ||
        expected16 != 65535u) return 7;
    if (poc_atomic16_exchange(&slot16.counter, 65535u) != 7u) return 8;
    if (poc_atomic16_fetch_add(&slot16.counter, 3u) != 65535u ||
        poc_atomic16_load(&slot16.counter) != 2u) return 9;

    poc_atomic8_store(&slot8.counter, 5u);
    poc_atomic16_store(&slot16.counter, 5u);
    for (i = 0; i < WORKERS; ++i)
        if (poc_thread_create(&workers[i], increment, NULL)) return 10;
    for (i = 0; i < WORKERS; ++i)
        if (poc_thread_join(&workers[i], &result) || result != 17) return 11;
    if (poc_atomic8_load(&slot8.counter) != (uint8_t)(5u + WORKERS * ITERATIONS) ||
        poc_atomic16_load(&slot16.counter) != (uint16_t)(5u + WORKERS * ITERATIONS)) return 12;
    if (slot8.before != 0xa5u || slot8.after != 0x5au ||
        slot16.before != 0xa55au || slot16.after != 0x5aa5u) return 13;

    if (poc_thread_create(&producer, publish, NULL)) return 14;
    for (i = 1; i <= PUBLISH; ++i) {
        while (!poc_atomic8_load(&signal8)) yield_cpu();
        if (packet != i) ++errors;
        poc_atomic8_store(&signal8, 0u);
    }
    for (i = 1; i <= PUBLISH; ++i) {
        while (poc_atomic16_load(&signal16) != (uint16_t)i) yield_cpu();
        if (packet != (i ^ 0x7654)) ++errors;
        poc_atomic16_store(&signal16, 0u);
    }
    if (poc_thread_join(&producer, &result) || result != 18 || errors) return 15;
#if defined(_WIN32) && defined(POC_ATOMIC64_EXTERNAL_DLL)
    if (poc_atomic64_backend_shutdown()) return 30;
#endif
    puts("atomic8/16: subword CAS/RMW, boundary guards, publication PASS");
    return 0;
}
