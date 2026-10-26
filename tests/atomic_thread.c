#include <stdint.h>
#include <stdio.h>
#include "poc/atomic32.h"
#ifdef _WIN32
#include <windows.h>
#define TEST_THREAD_RETURN DWORD WINAPI
#define TEST_THREAD_END return 0
#else
#include <pthread.h>
#include <sched.h>
#define TEST_THREAD_RETURN void *
#define TEST_THREAD_END return NULL
#endif
enum { WORKERS = 4, LOOPS = 20000, PUBLISH_LOOPS = 2000 };
static poc_atomic32 counter = POC_ATOMIC32_INIT(0);
static poc_atomic32 ready = POC_ATOMIC32_INIT(0);
static int published_payload;
static void test_yield(void) {
#ifdef _WIN32
    Sleep(0);
#else
    sched_yield();
#endif
}
static TEST_THREAD_RETURN counter_worker(void *ignored) {
    int i;
    (void)ignored;
    for (i = 0; i < LOOPS; ++i) (void)poc_atomic32_fetch_add(&counter, 1);
    TEST_THREAD_END;
}
static TEST_THREAD_RETURN publish_worker(void *ignored) {
    int i;
    (void)ignored;
    for (i = 1; i <= PUBLISH_LOOPS; ++i) {
        while (poc_atomic32_load(&ready) != 0) test_yield();
        published_payload = i;
        poc_atomic32_store(&ready, 1);
        while (poc_atomic32_load(&ready) != 0) test_yield();
    }
    TEST_THREAD_END;
}
#ifdef _WIN32
static int launch(HANDLE *h, LPTHREAD_START_ROUTINE fn) {
    *h = CreateThread(NULL, 0, fn, NULL, 0, NULL);
    return *h != NULL;
}
static int finish(HANDLE h) {
    DWORD wait = WaitForSingleObject(h, 30000);
    CloseHandle(h);
    return wait == WAIT_OBJECT_0;
}
#else
static int launch(pthread_t *h, void *(*fn)(void *)) { return pthread_create(h, NULL, fn, NULL) == 0; }
static int finish(pthread_t h) { return pthread_join(h, NULL) == 0; }
#endif
int main(void) {
    int i, errors = 0;
    uint32_t expected;
#ifdef _WIN32
    HANDLE thread[WORKERS], publisher;
#else
    pthread_t thread[WORKERS], publisher;
#endif
    if (sizeof(poc_atomic32) != 4) return 2;
    poc_atomic32_store(&counter, 7);
    expected = 8;
    if (poc_atomic32_compare_exchange(&counter, &expected, 9) || expected != 7) return 3;
    if (!poc_atomic32_compare_exchange(&counter, &expected, 9) || expected != 7) return 4;
    if (poc_atomic32_exchange(&counter, 0) != 9) return 5;
    if (poc_atomic32_fetch_add(&counter, 3) != 0 || poc_atomic32_load(&counter) != 3) return 6;
    poc_atomic32_store(&counter, 0);
    for (i = 0; i < WORKERS; ++i) if (!launch(&thread[i], counter_worker)) return 7;
    for (i = 0; i < WORKERS; ++i) if (!finish(thread[i])) return 8;
    if (poc_atomic32_load(&counter) != (uint32_t)(WORKERS * LOOPS)) return 9;
    if (!launch(&publisher, publish_worker)) return 10;
    for (i = 1; i <= PUBLISH_LOOPS; ++i) {
        while (poc_atomic32_load(&ready) != 1) test_yield();
        if (published_payload != i) ++errors;
        poc_atomic32_store(&ready, 0);
    }
    if (!finish(publisher) || errors) return 11;
    puts("atomic32: CAS, RMW, 4-worker increments, release/acquire publication OK");
    return 0;
}
