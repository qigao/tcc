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
