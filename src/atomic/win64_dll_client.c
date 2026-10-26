/* TinyCC Windows x64 client facade. llvm-mingw COFF .obj is deliberately
 * NOT passed to TinyCC's linker. A versioned DLL is the only linkage.
 * Init and shutdown are quiescent-only. No fallback, no lazy binding. */
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
static HMODULE module;
static load_fn native_load;
static store_fn native_store;
static rmw_fn native_exchange;
static rmw_fn native_add;
static cas_fn native_cas;
int poc_atomic64_backend_init(const char *path) {
    HMODULE candidate;
    version_fn version, storage;
    load_fn load;
    store_fn store;
    rmw_fn exchange, add;
    cas_fn cas;
    if (!path || module) return -1;
    candidate = LoadLibraryA(path);
    if (!candidate) return -1;
    version = (version_fn)GetProcAddress(candidate, "poc_win64_abi_version");
    storage = (version_fn)GetProcAddress(candidate, "poc_win64_storage_bytes");
    load = (load_fn)GetProcAddress(candidate, "poc_win64_load");
    store = (store_fn)GetProcAddress(candidate, "poc_win64_store");
    exchange = (rmw_fn)GetProcAddress(candidate, "poc_win64_exchange");
    add = (rmw_fn)GetProcAddress(candidate, "poc_win64_fetch_add");
    cas = (cas_fn)GetProcAddress(candidate, "poc_win64_compare_exchange");
    if (!version || version() != 1u || !storage ||
        storage() != 8u || sizeof(poc_atomic64) != 8u ||
        !load || !store || !exchange || !add || !cas) {
        FreeLibrary(candidate);
        return -1;
    }
    native_load = load;
    native_store = store;
    native_exchange = exchange;
    native_add = add;
    native_cas = cas;
    /* Publish only after the entire table is validated. There must be no
     * concurrent init, uses or unload in this deliberately narrow POC. */
    module = candidate;
    return 0;
}
int poc_atomic64_backend_shutdown(void) {
    HMODULE old = module;
    if (!old) return -1;
    module = NULL;
    native_load = NULL;
    native_store = NULL;
    native_exchange = NULL;
    native_add = NULL;
    native_cas = NULL;
    return FreeLibrary(old) ? 0 : -1;
}
static void require_backend(void) {
    if (!module) ExitProcess(125);
}
uint64_t poc_atomic64_load(const poc_atomic64 *p) {
    require_backend();
    return native_load(p);
}
void poc_atomic64_store(poc_atomic64 *p, uint64_t v) {
    require_backend();
    native_store(p, v);
}
uint64_t poc_atomic64_exchange(poc_atomic64 *p, uint64_t v) {
    require_backend();
    return native_exchange(p, v);
}
uint64_t poc_atomic64_fetch_add(poc_atomic64 *p, uint64_t delta) {
    require_backend();
    return native_add(p, delta);
}
int poc_atomic64_compare_exchange(poc_atomic64 *p, uint64_t *e, uint64_t d) {
    require_backend();
    return native_cas(p, e, d);
}
