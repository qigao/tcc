/* 32-bit Windows TinyCC C client for llvm-mingw i686 DLL.
 * The caller and provider exchange fixed-width scalars through cdecl
 * functions only; no CRT buffers, C++ objects or COFF .obj are shared.
 * Init/unload are QUIESCENT ONLY. No concurrent unload is qualified here. */
#include <windows.h>
#include <stdint.h>
#include "poc/atomic_small.h"
#include "poc/win_x86_atomic.h"
typedef unsigned (__cdecl *version_fn)(void);
typedef int (__cdecl *dispatch_fn)(
    void *, unsigned, unsigned, unsigned *, unsigned);
/* Single 32-bit Interlocked CAS state: phase in low two bits, number of
 * outstanding leases in higher bits (multiples of four).
 * The full-barrier CAS is the linearization point for every call admission. */
enum { X86_EMPTY = 0, X86_READY = 1, X86_DRAIN = 2,
       X86_MUTATING = 3, X86_STEP = 4 };
static volatile LONG g_state = X86_EMPTY;
static unsigned g_generation;
static HMODULE g_module;
static dispatch_fn g_dispatch;
static LONG state_read(void) {
    return InterlockedCompareExchange(&g_state, 0, 0);
}
static int enter_dll(void) {
    for (;;) {
        LONG state = state_read();
        if ((state & 3) != X86_READY) return POC_X86_ATOMIC_BUSY;
        if ((unsigned)(state / X86_STEP) >= POC_X86_ATOMIC_MAX_LEASES)
            return POC_X86_ATOMIC_BUSY;
        if (InterlockedCompareExchange(&g_state, state + X86_STEP, state) == state)
            return 0;
    }
}
static void leave_dll(void) {
    for (;;) {
        LONG state = state_read();
        if ((state / X86_STEP) <= 0 || (state & 3) == X86_EMPTY ||
            (state & 3) == X86_MUTATING) ExitProcess(124);
        if (InterlockedCompareExchange(&g_state, state - X86_STEP, state) == state)
            return;
    }
}
static FARPROC lookup(HMODULE module, const char *undecorated,
                       const char *decorated) {
    FARPROC p = GetProcAddress(module, undecorated);
    return p ? p : GetProcAddress(module, decorated);
}
int poc_x86_atomic_backend_init(const char *path) {
    HMODULE candidate;
    version_fn version, layout;
    dispatch_fn dispatcher;
    if (!path ||
        InterlockedCompareExchange(&g_state, X86_MUTATING, X86_EMPTY) != X86_EMPTY)
        return -1;
    candidate = LoadLibraryA(path);
    if (!candidate) {
        InterlockedExchange(&g_state, X86_EMPTY);
        return -1;
    }
    version = (version_fn)lookup(candidate,
                 "poc_x86_atomic_version", "_poc_x86_atomic_version");
    layout = (version_fn)lookup(candidate,
                 "poc_x86_atomic_layout", "_poc_x86_atomic_layout");
    dispatcher = (dispatch_fn)lookup(candidate,
                 "poc_x86_atomic_dispatch", "_poc_x86_atomic_dispatch");
    if (!version || version() != 1u || !layout ||
        layout() != (unsigned)(sizeof(poc_atomic8) |
                              (sizeof(poc_atomic16) << 8) |
                              (sizeof(poc_atomic_ptr32) << 16)) ||
        sizeof(void *) != 4u || !dispatcher) {
        (void)FreeLibrary(candidate);
        InterlockedExchange(&g_state, X86_EMPTY);
        return -1;
    }
    g_dispatch = dispatcher;
    g_module = candidate;
    ++g_generation;
    if (!g_generation) ++g_generation;
    InterlockedExchange(&g_state, X86_READY);
    return 0;
}
int poc_x86_atomic_backend_borrow(poc_x86_atomic_lease *token) {
    if (!token || token->active) return -1;
    if (enter_dll()) return POC_X86_ATOMIC_BUSY;
    token->generation = g_generation;
    token->active = 1;
    return 0;
}
int poc_x86_atomic_backend_return(poc_x86_atomic_lease *token) {
    if (!token || token->active != 1 ||
        token->generation != g_generation) return -1;
    token->active = 0;
    leave_dll();
    return 0;
}
int poc_x86_atomic_backend_shutdown(void) {
    LONG state;
    HMODULE previous;
    for (;;) {
        state = state_read();
        if (state == X86_EMPTY || state == X86_MUTATING) return -1;
        if ((state & 3) == X86_READY) {
            if (InterlockedCompareExchange(&g_state, state + 1, state) != state)
                continue;
            ++state;
        }
        /* Admission is now closed: even if clients return leases, new
         * entry through GetProcAddress is forbidden. */
        if ((state & ~3L) != 0) return POC_X86_ATOMIC_BUSY;
        if (InterlockedCompareExchange(&g_state, X86_MUTATING, X86_DRAIN) != X86_DRAIN)
            continue;
        previous = g_module;
        if (!FreeLibrary(previous)) {
            InterlockedExchange(&g_state, X86_DRAIN);
            return -1;
        }
        g_module = NULL;
        g_dispatch = NULL;
        InterlockedExchange(&g_state, X86_EMPTY);
        return 0;
    }
}
static int call(void *slot, unsigned width, unsigned op,
                unsigned *observed, unsigned desired) {
    int result;
    if (!slot || !observed) ExitProcess(126);
    if (enter_dll()) ExitProcess(125);
    /* The lease acquired above pins the DLL and dispatch table
     * until after the DLL function has returned. */
    result = g_dispatch(slot, width, op, observed, desired);
    leave_dll();
    if (result < 0) ExitProcess(126);
    return result;
}
uint8_t poc_atomic8_load(const poc_atomic8 *p) {
    unsigned v = 0;
    (void)call((void *)&p->value, 1u, 1u, &v, 0u);
    return (uint8_t)v;
}
void poc_atomic8_store(poc_atomic8 *p, uint8_t v) {
    unsigned discarded = 0;
    (void)call((void *)&p->value, 1u, 2u, &discarded, v);
}
uint8_t poc_atomic8_exchange(poc_atomic8 *p, uint8_t v) {
    unsigned previous = 0;
    (void)call((void *)&p->value, 1u, 3u, &previous, v);
    return (uint8_t)previous;
}
uint8_t poc_atomic8_fetch_add(poc_atomic8 *p, uint8_t v) {
    unsigned previous = 0;
    (void)call((void *)&p->value, 1u, 4u, &previous, v);
    return (uint8_t)previous;
}
int poc_atomic8_compare_exchange(poc_atomic8 *p, uint8_t *expected, uint8_t desired) {
    unsigned v;
    int result;
    if (!expected) ExitProcess(126);
    v = *expected;
    result = call((void *)&p->value, 1u, 5u, &v, desired);
    *expected = (uint8_t)v;
    return result;
}
uint16_t poc_atomic16_load(const poc_atomic16 *p) {
    unsigned v = 0;
    (void)call((void *)&p->value, 2u, 1u, &v, 0u);
    return (uint16_t)v;
}
void poc_atomic16_store(poc_atomic16 *p, uint16_t v) {
    unsigned discarded = 0;
    (void)call((void *)&p->value, 2u, 2u, &discarded, v);
}
uint16_t poc_atomic16_exchange(poc_atomic16 *p, uint16_t v) {
    unsigned previous = 0;
    (void)call((void *)&p->value, 2u, 3u, &previous, v);
    return (uint16_t)previous;
}
uint16_t poc_atomic16_fetch_add(poc_atomic16 *p, uint16_t v) {
    unsigned previous = 0;
    (void)call((void *)&p->value, 2u, 4u, &previous, v);
    return (uint16_t)previous;
}
int poc_atomic16_compare_exchange(
    poc_atomic16 *p, uint16_t *expected, uint16_t desired) {
    unsigned v;
    int result;
    if (!expected) ExitProcess(126);
    v = *expected;
    result = call((void *)&p->value, 2u, 5u, &v, desired);
    *expected = (uint16_t)v;
    return result;
}
void *poc_atomic_ptr32_load(const poc_atomic_ptr32 *p) {
    unsigned v = 0;
    (void)call((void *)&p->bits, 4u, 1u, &v, 0u);
    return (void *)(uintptr_t)v;
}
void poc_atomic_ptr32_store(poc_atomic_ptr32 *p, void *ptr) {
    unsigned discarded = 0;
    (void)call((void *)&p->bits, 4u, 2u, &discarded,
               (unsigned)(uintptr_t)ptr);
}
void *poc_atomic_ptr32_exchange(poc_atomic_ptr32 *p, void *ptr) {
    unsigned previous = 0;
    (void)call((void *)&p->bits, 4u, 3u, &previous,
               (unsigned)(uintptr_t)ptr);
    return (void *)(uintptr_t)previous;
}
int poc_atomic_ptr32_compare_exchange(
    poc_atomic_ptr32 *p, void **expected, void *desired) {
    unsigned previous;
    int result;
    if (!expected) ExitProcess(126);
    previous = (unsigned)(uintptr_t)*expected;
    result = call((void *)&p->bits, 4u, 5u, &previous,
                  (unsigned)(uintptr_t)desired);
    *expected = (void *)(uintptr_t)previous;
    return result;
}
