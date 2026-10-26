#ifndef TCC_POC_WIN_X86_ATOMIC_H
#define TCC_POC_WIN_X86_ATOMIC_H
#include <stdint.h>
#if !defined(_WIN32) || !(defined(__i386__) || defined(_M_IX86))
#error "Only the 32-bit Windows ABI is supported by this facade"
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Independent 4-byte pointer storage, not _Atomic(void*) or a Salts record. */
typedef struct poc_atomic_ptr32 {
    volatile uint32_t bits;
} poc_atomic_ptr32;
#define POC_ATOMIC_PTR32_INIT_NULL {0u}
/* Explicit bounded DLL ownership. Token storage is stable and held by
 * one thread only; never copy a live token or use it after return.
 * Every C ABI dispatch call also holds an internal lease. */
#define POC_X86_ATOMIC_BUSY (-2)
#define POC_X86_ATOMIC_MAX_LEASES 64
typedef struct poc_x86_atomic_lease {
    unsigned generation;
    int active;
} poc_x86_atomic_lease;
#define POC_X86_ATOMIC_LEASE_INIT { 0u, 0 }
int poc_x86_atomic_backend_init(const char *path);
int poc_x86_atomic_backend_borrow(poc_x86_atomic_lease *token);
int poc_x86_atomic_backend_return(poc_x86_atomic_lease *token);
int poc_x86_atomic_backend_shutdown(void);
void *poc_atomic_ptr32_load(const poc_atomic_ptr32 *p);
void poc_atomic_ptr32_store(poc_atomic_ptr32 *p, void *value);
void *poc_atomic_ptr32_exchange(poc_atomic_ptr32 *p, void *value);
int poc_atomic_ptr32_compare_exchange(
    poc_atomic_ptr32 *p, void **expected, void *desired);
#ifdef __cplusplus
}
#endif
#endif
