/* TinyCC x64 C facade. llvm-mingw owns the COFF/GAS implementation
 * behind a DLL C ABI, never a TinyCC-linked llvm-mingw object.
 *
 * Atomic LONG state (full-barrier InterlockedCompareExchange):
 *   low 2 bits: 0 EMPTY, 1 READY, 2 DRAIN, 3 MUTATING (init/unload)
 *   high bits:   current lease count * 4
 *
 * Enter performs ONE CAS that pins the DLL before acquiring any function
 * pointer. Shutdown atomically closes admission and NEVER calls FreeLibrary
 * while active leases remain. New operations fail closed during drain.
 * All Windows Interlocked routines below are supported by the pinned TCC
 * toolchain; no non-atomic fallback, hidden dynamic worker, or timeout.
 */
#include <windows.h>
#include <stdint.h>
#include "poc/atomic64.h"
#if !defined(_WIN64) && !defined(__x86_64__)
#error "DLL client requires Windows x64"
#endif
typedef unsigned (__cdecl *version_fn)(void);
typedef uint64_t (__cdecl *load_fn)(const poc_atomic64 *);
typedef void (__cdecl *store_fn)(poc_atomic64 *, uint64_t);
typedef uint64_t (__cdecl *rmw_fn)(poc_atomic64 *, uint64_t);
typedef int (__cdecl *cas_fn)(poc_atomic64 *, uint64_t *, uint64_t);
enum { EMPTY = 0, READY = 1, DRAIN = 2, MUTATING = 3, LEASE_STEP = 4 };
static volatile LONG g_admission = EMPTY;
static unsigned g_generation;
static HMODULE g_module;
static load_fn g_load;
static store_fn g_store;
static rmw_fn g_exchange;
static rmw_fn g_add;
static cas_fn g_cas;

static LONG admission_read(void) {
    return InterlockedCompareExchange(&g_admission, 0, 0);
}
static int admission_enter(void) {
    for (;;) {
        LONG s = admission_read();
        if ((s & 3) != READY) return POC_ATOMIC64_BUSY;
        if ((unsigned)(s / LEASE_STEP) >= POC_ATOMIC64_MAX_LEASES)
            return POC_ATOMIC64_BUSY;
        if (InterlockedCompareExchange(&g_admission, s + LEASE_STEP, s) == s)
            return 0;
    }
}
static void admission_leave(void) {
    for (;;) {
        LONG s = admission_read();
        if ((s / LEASE_STEP) <= 0 || (s & 3) == MUTATING ||
            (s & 3) == EMPTY) ExitProcess(124);
        if (InterlockedCompareExchange(&g_admission, s - LEASE_STEP, s) == s)
            return;
    }
}
int poc_atomic64_backend_init(const char *path) {
    HMODULE candidate;
    version_fn version, storage;
    load_fn load;
    store_fn store;
    rmw_fn exchange, add;
    cas_fn cas;
    if (!path || InterlockedCompareExchange(&g_admission, MUTATING, EMPTY) != EMPTY)
        return -1;
    candidate = LoadLibraryA(path);
    if (!candidate) {
        InterlockedExchange(&g_admission, EMPTY);
        return -1;
    }
    version = (version_fn)GetProcAddress(candidate, "poc_win64_abi_version");
    storage = (version_fn)GetProcAddress(candidate, "poc_win64_storage_bytes");
    load = (load_fn)GetProcAddress(candidate, "poc_win64_load");
    store = (store_fn)GetProcAddress(candidate, "poc_win64_store");
    exchange = (rmw_fn)GetProcAddress(candidate, "poc_win64_exchange");
    add = (rmw_fn)GetProcAddress(candidate, "poc_win64_fetch_add");
    cas = (cas_fn)GetProcAddress(candidate, "poc_win64_compare_exchange");
    if (!version || version() != 1u || !storage || storage() != 8u ||
        sizeof(poc_atomic64) != 8u || !load || !store || !exchange ||
        !add || !cas) {
        FreeLibrary(candidate);
        InterlockedExchange(&g_admission, EMPTY);
        return -1;
    }
    g_module = candidate;
    g_load = load;
    g_store = store;
    g_exchange = exchange;
    g_add = add;
    g_cas = cas;
    ++g_generation;
    if (g_generation == 0u) ++g_generation;
    InterlockedExchange(&g_admission, READY);
    return 0;
}
int poc_atomic64_backend_borrow(poc_atomic64_lease *token) {
    if (!token || token->active) return -1;
    if (admission_enter()) return POC_ATOMIC64_BUSY;
    token->generation = g_generation;
    token->active = 1;
    return 0;
}
int poc_atomic64_backend_return(poc_atomic64_lease *token) {
    if (!token || token->active != 1 ||
        token->generation != g_generation) return -1;
    token->active = 0;
    admission_leave();
    return 0;
}
int poc_atomic64_backend_shutdown(void) {
    LONG s;
    HMODULE old;
    for (;;) {
        s = admission_read();
        if (s == EMPTY || s == MUTATING) return -1;
        if ((s & 3) == READY) {
            if (InterlockedCompareExchange(&g_admission, s + 1, s) != s)
                continue;
            s += 1;
        }
        /* Once draining begins, no new lease or call may enter. */
        if ((s & ~3L) != 0) return POC_ATOMIC64_BUSY;
        if (InterlockedCompareExchange(&g_admission, MUTATING, DRAIN) != DRAIN)
            continue;
        old = g_module;
        if (!FreeLibrary(old)) {
            InterlockedExchange(&g_admission, DRAIN);
            return -1;
        }
        g_module = NULL;
        g_load = NULL;
        g_store = NULL;
        g_exchange = NULL;
        g_add = NULL;
        g_cas = NULL;
        InterlockedExchange(&g_admission, EMPTY);
        return 0;
    }
}
static void require_admitted(void) {
    if (admission_enter()) ExitProcess(125);
}
uint64_t poc_atomic64_load(const poc_atomic64 *p) {
    uint64_t result;
    require_admitted();
    result = g_load(p);
    admission_leave();
    return result;
}
void poc_atomic64_store(poc_atomic64 *p, uint64_t v) {
    require_admitted();
    g_store(p, v);
    admission_leave();
}
uint64_t poc_atomic64_exchange(poc_atomic64 *p, uint64_t v) {
    uint64_t result;
    require_admitted();
    result = g_exchange(p, v);
    admission_leave();
    return result;
}
uint64_t poc_atomic64_fetch_add(poc_atomic64 *p, uint64_t delta) {
    uint64_t result;
    require_admitted();
    result = g_add(p, delta);
    admission_leave();
    return result;
}
int poc_atomic64_compare_exchange(poc_atomic64 *p, uint64_t *e, uint64_t d) {
    int result;
    require_admitted();
    result = g_cas(p, e, d);
    admission_leave();
    return result;
}
