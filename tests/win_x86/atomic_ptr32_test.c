/* Real Windows x86 TinyCC => llvm-mingw i686 DLL pointer CAS test.
 * Only the client owns pointed-to objects. Unload after producer join.
 * No pointer-value scaling and no _Atomic(T) layout reinterpretation. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "poc/win_x86_atomic.h"
#include "poc/threads.h"
#if !defined(__i386__) || !defined(_WIN32)
#error "Requires a real Windows x86 TinyCC client"
#endif
enum { PUBLICATIONS = 2400 };
typedef struct message {
    unsigned sequence;
    unsigned guard;
} message;
static message shared_message;
static poc_atomic_ptr32 mailbox = POC_ATOMIC_PTR32_INIT_NULL;
static int left = 5, right = 17;
static HANDLE worker_entered, worker_release;
static int lease_holder(void *unused) {
    poc_x86_atomic_lease lease = POC_X86_ATOMIC_LEASE_INIT;
    (void)unused;
    if (poc_x86_atomic_backend_borrow(&lease)) return 81;
    if (!SetEvent(worker_entered)) return 82;
    if (WaitForSingleObject(worker_release, 30000) != WAIT_OBJECT_0) return 83;
    if (poc_x86_atomic_backend_return(&lease)) return 84;
    if (poc_x86_atomic_backend_return(&lease) != -1) return 85;
    return 86;
}
static int producer(void *unused) {
    unsigned i;
    (void)unused;
    for (i = 1; i <= PUBLICATIONS; ++i) {
        void *expected;
        while (poc_atomic_ptr32_load(&mailbox) != NULL) Sleep(0);
        shared_message.sequence = i;
        shared_message.guard = i ^ 0x716d2b39u;
        expected = NULL;
        while (!poc_atomic_ptr32_compare_exchange(
                    &mailbox, &expected, &shared_message)) {
            expected = NULL;
            Sleep(0);
        }
        while (poc_atomic_ptr32_load(&mailbox) != NULL) Sleep(0);
    }
    return 73;
}
int main(void) {
    poc_thread t = POC_THREAD_INIT;
    unsigned i, errors = 0;
    void *expected;
    int result;
    if (sizeof(void *) != 4u || sizeof(mailbox) != 4u) return 1;
    if (poc_x86_atomic_backend_init("nonexistent_x86_atomic.dll") != -1) return 2;
    if (poc_x86_atomic_backend_init("poc_x86_atomic_badabi.dll") != -1) return 3;
    if (GetModuleHandleA("poc_x86_atomic_badabi.dll") != NULL) return 4;
    if (poc_x86_atomic_backend_init("poc_x86_atomic.dll")) return 5;
    if (poc_x86_atomic_backend_init("poc_x86_atomic.dll") != -1) return 6;
    if (poc_atomic_ptr32_load(&mailbox) != NULL) return 7;
    expected = &left;
    if (poc_atomic_ptr32_compare_exchange(&mailbox, &expected, &right) ||
        expected != NULL) return 8;
    if (!poc_atomic_ptr32_compare_exchange(&mailbox, &expected, &left) ||
        expected != NULL) return 9;
    if (poc_atomic_ptr32_exchange(&mailbox, &right) != &left) return 10;
    expected = &left;
    if (poc_atomic_ptr32_compare_exchange(&mailbox, &expected, NULL) ||
        expected != &right) return 11;
    if (!poc_atomic_ptr32_compare_exchange(&mailbox, &expected, NULL) ||
        expected != &right) return 12;
    if (poc_thread_create(&t, producer, NULL)) return 13;
    for (i = 1; i <= PUBLICATIONS; ++i) {
        message *m;
        while ((m = (message *)poc_atomic_ptr32_load(&mailbox)) == NULL) Sleep(0);
        if (m != &shared_message || m->sequence != i ||
            m->guard != (i ^ 0x716d2b39u)) ++errors;
        poc_atomic_ptr32_store(&mailbox, NULL);
    }
    if (poc_thread_join(&t, &result) || result != 73 || errors) return 14;
    {
        poc_x86_atomic_lease tokens[POC_X86_ATOMIC_MAX_LEASES] = {{0}};
        poc_x86_atomic_lease extra = POC_X86_ATOMIC_LEASE_INIT;
        poc_thread holding = POC_THREAD_INIT;
        int worker_result;
        for (i = 0; i < POC_X86_ATOMIC_MAX_LEASES; ++i)
            if (poc_x86_atomic_backend_borrow(&tokens[i])) return 20;
        if (poc_x86_atomic_backend_borrow(&extra) != POC_X86_ATOMIC_BUSY)
            return 21;
        for (i = 0; i < POC_X86_ATOMIC_MAX_LEASES; ++i)
            if (poc_x86_atomic_backend_return(&tokens[i])) return 22;
        if (poc_x86_atomic_backend_return(&tokens[0]) != -1) return 23;
        worker_entered = CreateEventA(NULL, TRUE, FALSE, NULL);
        worker_release = CreateEventA(NULL, TRUE, FALSE, NULL);
        if (!worker_entered || !worker_release) return 24;
        if (poc_thread_create(&holding, lease_holder, NULL)) return 25;
        if (WaitForSingleObject(worker_entered, 30000) != WAIT_OBJECT_0) return 26;
        if (poc_x86_atomic_backend_shutdown() != POC_X86_ATOMIC_BUSY) return 27;
        if (poc_x86_atomic_backend_borrow(&extra) != POC_X86_ATOMIC_BUSY)
            return 28;
        if (poc_x86_atomic_backend_init("poc_x86_atomic.dll") != -1) return 29;
        if (GetModuleHandleA("poc_x86_atomic.dll") == NULL) return 30;
        if (!SetEvent(worker_release)) return 31;
        if (poc_thread_join(&holding, &worker_result) || worker_result != 86)
            return 32;
        if (!CloseHandle(worker_entered) || !CloseHandle(worker_release))
            return 33;
    }
    if (poc_x86_atomic_backend_shutdown() ||
        GetModuleHandleA("poc_x86_atomic.dll") != NULL) return 15;
    if (poc_x86_atomic_backend_shutdown() != -1) return 16;
    if (poc_x86_atomic_backend_init("poc_x86_atomic.dll")) return 17;
    poc_atomic_ptr32_store(&mailbox, &left);
    if (poc_atomic_ptr32_load(&mailbox) != &left) return 18;
    if (poc_x86_atomic_backend_shutdown()) return 19;
    puts("win-x86-tcc: 8/16 DLL ABI + pointer32 CAS/publication, unload/reopen PASS");
    return 0;
}
