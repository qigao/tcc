#ifndef TCC_POC_ATOMIC64_H
#define TCC_POC_ATOMIC64_H
#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
/* Explicit 64-bit-only POC, not generic _Atomic or a Salts public ABI. */
#if !defined(UINTPTR_MAX) || UINTPTR_MAX != UINT64_MAX
#error "atomic64/pointer POC requires a native 64-bit pointer ABI"
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Pointer member ensures 8-byte alignment on the supported x64/ARM64 ABIs.
 * Only 'value' is accessed; never use the alignment member after init. */
typedef union poc_atomic64 {
    volatile uint64_t value;
    void *alignment;
} poc_atomic64;
#define POC_ATOMIC64_INIT(v_) { (v_) }
uint64_t poc_atomic64_load(const poc_atomic64 *p);
void poc_atomic64_store(poc_atomic64 *p, uint64_t v);
uint64_t poc_atomic64_exchange(poc_atomic64 *p, uint64_t v);
uint64_t poc_atomic64_fetch_add(poc_atomic64 *p, uint64_t delta);
int poc_atomic64_compare_exchange(poc_atomic64 *p, uint64_t *expected, uint64_t desired);

/* No pointer object shares layout with a compiler's C11 _Atomic(void*).
 * Conversion is roundtrip via uintptr_t on supported flat 64-bit platforms.
 * Lifetime of pointed-to objects remains a separate caller obligation. */
typedef struct poc_atomic_ptr {
    poc_atomic64 bits;
} poc_atomic_ptr;
#define POC_ATOMIC_PTR_INIT_NULL { { 0 } }
void *poc_atomic_ptr_load(const poc_atomic_ptr *p);
void poc_atomic_ptr_store(poc_atomic_ptr *p, void *v);
void *poc_atomic_ptr_exchange(poc_atomic_ptr *p, void *v);
int poc_atomic_ptr_compare_exchange(poc_atomic_ptr *p, void **expected, void *desired);
#if defined(_WIN32) && defined(POC_ATOMIC64_EXTERNAL_DLL)
/* Private Windows TinyCC DLL owner API, NOT generic C11 atomic ABI.
 * Admission bounded to 64 simultaneous leases/calls; acquire returns BUSY
 * once draining starts. The caller owns each lease token; never copy/move it.
 * Closing is cancel-first: refuse new work, wait for existing lease returns.
 * Init and final DLL re-init are serialized through the state machine.
 * No C++ objects or allocators cross this boundary. */
#define POC_ATOMIC64_BUSY (-2)
#define POC_ATOMIC64_MAX_LEASES 64
typedef struct poc_atomic64_lease {
    unsigned generation;
    int active;
} poc_atomic64_lease;
#define POC_ATOMIC64_LEASE_INIT { 0u, 0 }
int poc_atomic64_backend_init(const char *dll_path);
int poc_atomic64_backend_borrow(poc_atomic64_lease *token);
int poc_atomic64_backend_return(poc_atomic64_lease *token);
int poc_atomic64_backend_shutdown(void);
#endif
#ifdef __cplusplus
}
#endif
#endif
