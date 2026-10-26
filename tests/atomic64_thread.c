#include "poc/atomic64.h"
#include "poc/threads.h"
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#endif
enum { WORKERS = 4, ITERATIONS = 16000, PUBLICATIONS = 1800 };
typedef struct packet { int sequence; int guard; } packet;
static packet shared_packet;
static poc_atomic_ptr mailbox = POC_ATOMIC_PTR_INIT_NULL;
static poc_atomic64 counter = POC_ATOMIC64_INIT(UINT64_C(0x00000001ffffff00));

static void yield_cpu(void) {
#ifdef _WIN32
    Sleep(0);
#else
    sched_yield();
#endif
}
static int increment_worker(void *argument) {
    int i;
    (void)argument;
    for (i = 0; i < ITERATIONS; ++i)
        (void)poc_atomic64_fetch_add(&counter, UINT64_C(1));
    return 7;
}
static int publish_worker(void *argument) {
    int i;
    (void)argument;
    for (i = 0; i < PUBLICATIONS; ++i) {
        while (poc_atomic_ptr_load(&mailbox) != NULL) yield_cpu();
        shared_packet.sequence = i;
        shared_packet.guard = i ^ 0x5a5a;
        poc_atomic_ptr_store(&mailbox, &shared_packet);
        while (poc_atomic_ptr_load(&mailbox) != NULL) yield_cpu();
    }
    return 9;
}
#if defined(_WIN32) && defined(POC_ATOMIC64_EXTERNAL_DLL)
static HANDLE borrow_entered;
static HANDLE borrow_release;
static int hold_backend_lease(void *unused) {
    poc_atomic64_lease token = POC_ATOMIC64_LEASE_INIT;
    (void)unused;
    if (poc_atomic64_backend_borrow(&token) != 0) return 81;
    if (!SetEvent(borrow_entered)) return 82;
    if (WaitForSingleObject(borrow_release, 30000) != WAIT_OBJECT_0) return 83;
    if (poc_atomic64_backend_return(&token) != 0) return 84;
    if (poc_atomic64_backend_return(&token) != -1) return 85;
    return 86;
}
#endif

int main(void) {
    int i, result, err = 0;
    poc_thread workers[WORKERS] = {{0}}, publisher = POC_THREAD_INIT;
    uint64_t before = UINT64_C(0x00000001ffffff00), expected;
    int a = 17, b = 23;
    void *p;
#if defined(_WIN32) && defined(POC_ATOMIC64_EXTERNAL_DLL)
    if (poc_atomic64_backend_init("missing-poc-atomic64.dll") != -1) return 21;
    /* A real on-disk DLL with wrong ABI must be unloaded on rejection. */
    if (poc_atomic64_backend_init("poc_atomic64_badabi.dll") != -1) return 26;
    if (GetModuleHandleA("poc_atomic64_badabi.dll") != NULL) return 27;
    if (poc_atomic64_backend_init("poc_atomic64_win.dll") != 0) return 22;
    if (poc_atomic64_backend_init("poc_atomic64_win.dll") != -1) return 23;
#endif
    if (sizeof(poc_atomic64) != 8 || sizeof(poc_atomic_ptr) != 8) return 1;
    if (sizeof(void *) != 8 || ((uintptr_t)&counter & 7u) != 0) return 2;
    if (poc_atomic64_load(&counter) != before) return 3;
    expected = before + 1;
    if (poc_atomic64_compare_exchange(&counter, &expected, 999) || expected != before) return 4;
    if (!poc_atomic64_compare_exchange(&counter, &expected, before + 3) || expected != before) return 5;
    if (poc_atomic64_exchange(&counter, before) != before + 3) return 6;
    if (poc_atomic64_fetch_add(&counter, UINT64_C(0x200000001)) != before) return 7;
    if (poc_atomic64_load(&counter) != before + UINT64_C(0x200000001)) return 8;
    poc_atomic64_store(&counter, before);

    p = &a;
    if (poc_atomic_ptr_compare_exchange(&mailbox, &p, &b) || p != NULL) return 9;
    if (!poc_atomic_ptr_compare_exchange(&mailbox, &p, &a) || p != NULL) return 10;
    if (poc_atomic_ptr_load(&mailbox) != &a) return 11;
    if (poc_atomic_ptr_exchange(&mailbox, &b) != &a) return 12;
    p = &a;
    if (poc_atomic_ptr_compare_exchange(&mailbox, &p, NULL) || p != &b) return 13;
    if (!poc_atomic_ptr_compare_exchange(&mailbox, &p, NULL) || p != &b) return 14;
    if (poc_atomic_ptr_load(&mailbox) != NULL) return 15;

    for (i = 0; i < WORKERS; ++i)
        if (poc_thread_create(&workers[i], increment_worker, NULL)) return 16;
    for (i = 0; i < WORKERS; ++i)
        if (poc_thread_join(&workers[i], &result) || result != 7) return 17;
    if (poc_atomic64_load(&counter) != before + WORKERS * ITERATIONS) return 18;
    if (poc_thread_create(&publisher, publish_worker, NULL)) return 19;
    for (i = 0; i < PUBLICATIONS; ++i) {
        packet *current;
        while ((current = (packet *)poc_atomic_ptr_load(&mailbox)) == NULL) yield_cpu();
        if (current != &shared_packet ||
            current->sequence != i || current->guard != (i ^ 0x5a5a)) ++err;
        poc_atomic_ptr_store(&mailbox, NULL);
    }
    if (poc_thread_join(&publisher, &result) || result != 9 || err) return 20;
#if defined(_WIN32) && defined(POC_ATOMIC64_EXTERNAL_DLL)
    {
        poc_atomic64_lease tokens[POC_ATOMIC64_MAX_LEASES] = {{0}};
        poc_atomic64_lease extra = POC_ATOMIC64_LEASE_INIT;
        poc_thread holding = POC_THREAD_INIT;
        int hold_result;
        for (i = 0; i < POC_ATOMIC64_MAX_LEASES; ++i)
            if (poc_atomic64_backend_borrow(&tokens[i]) != 0) return 32;
        if (poc_atomic64_backend_borrow(&extra) != POC_ATOMIC64_BUSY) return 33;
        for (i = 0; i < POC_ATOMIC64_MAX_LEASES; ++i)
            if (poc_atomic64_backend_return(&tokens[i]) != 0) return 34;
        if (poc_atomic64_backend_return(&tokens[0]) != -1) return 35;
        borrow_entered = CreateEventA(NULL, TRUE, FALSE, NULL);
        borrow_release = CreateEventA(NULL, TRUE, FALSE, NULL);
        if (!borrow_entered || !borrow_release) return 36;
        if (poc_thread_create(&holding, hold_backend_lease, NULL)) return 37;
        if (WaitForSingleObject(borrow_entered, 30000) != WAIT_OBJECT_0) return 38;
        /* Close must reject fresh calls while a real worker holds a lease,
         * and must not unload the DLL until that borrower has returned it. */
        if (poc_atomic64_backend_shutdown() != POC_ATOMIC64_BUSY) return 39;
        if (poc_atomic64_backend_borrow(&extra) != POC_ATOMIC64_BUSY) return 40;
        if (poc_atomic64_backend_init("poc_atomic64_win.dll") != -1) return 41;
        if (GetModuleHandleA("poc_atomic64_win.dll") == NULL) return 42;
        if (!SetEvent(borrow_release)) return 43;
        if (poc_thread_join(&holding, &hold_result) || hold_result != 86) return 44;
        if (!CloseHandle(borrow_entered) || !CloseHandle(borrow_release)) return 45;
    }
    if (poc_atomic64_backend_shutdown() != 0) return 24;
    if (GetModuleHandleA("poc_atomic64_win.dll") != NULL) return 28;
    if (poc_atomic64_backend_shutdown() != -1) return 25;
    /* Reopen only after all workers are joined; verify no stale entrypoints. */
    if (poc_atomic64_backend_init("poc_atomic64_win.dll") != 0) return 29;
    poc_atomic64_store(&counter, UINT64_C(0xfedcba9876543210));
    if (poc_atomic64_load(&counter) != UINT64_C(0xfedcba9876543210)) return 30;
    if (poc_atomic64_backend_shutdown() != 0 ||
        GetModuleHandleA("poc_atomic64_win.dll") != NULL) return 31;
#endif
    puts("atomic64/ptr: high-bit CAS/RMW, MPMC increment, pointer publication PASS");
    return 0;
}
