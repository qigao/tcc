/* Hardened Runtime diagnostic, NOT an app/notarization acceptance.
 * No entitlement disables library validation, and no DYLD_* override.
 * The app only loads the ad-hoc signed dylib by its absolute path. */
#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include "poc/atomic32.h"
typedef uint32_t (*load_fn)(const poc_atomic32 *);
typedef uint32_t (*add_fn)(poc_atomic32 *, uint32_t);
int main(int argc, char **argv) {
    void *module;
    load_fn load;
    add_fn add;
    poc_atomic32 counter = POC_ATOMIC32_INIT(2u);
    int ok;
    if (argc != 2) return 1;
    module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!module) {
        fprintf(stderr, "Strict hardened dlopen failed: %s\n", dlerror());
        return 2;
    }
    load = (load_fn)dlsym(module, "poc_atomic32_load");
    add = (add_fn)dlsym(module, "poc_atomic32_fetch_add");
    ok = load && add && add(&counter, 5) == 2u && load(&counter) == 7u;
    if (dlclose(module) != 0) return 3;
    if (!ok) return 4;
    puts("macOS hardened ad-hoc signing / strict Clang dylib ABI PASS");
    return 0;
}
