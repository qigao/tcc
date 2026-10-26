/* All executable atomic operations live inside the llvm-mingw DLL.
 * Only fixed-width C ABI types cross this library boundary. */
#include "poc/atomic64.h"
#ifdef _WIN32
#define POC_EXPORT __declspec(dllexport)
#define POC_CALL __cdecl
#else
#error "This export module is Windows-only"
#endif
POC_EXPORT unsigned POC_CALL poc_win64_abi_version(void) { return 1u; }
POC_EXPORT unsigned POC_CALL poc_win64_storage_bytes(void) {
    return (unsigned)sizeof(poc_atomic64);
}
POC_EXPORT uint64_t POC_CALL poc_win64_load(const poc_atomic64 *p) {
    return poc_atomic64_load(p);
}
POC_EXPORT void POC_CALL poc_win64_store(poc_atomic64 *p, uint64_t value) {
    poc_atomic64_store(p, value);
}
POC_EXPORT uint64_t POC_CALL poc_win64_exchange(poc_atomic64 *p, uint64_t value) {
    return poc_atomic64_exchange(p, value);
}
POC_EXPORT uint64_t POC_CALL poc_win64_fetch_add(poc_atomic64 *p, uint64_t delta) {
    return poc_atomic64_fetch_add(p, delta);
}
POC_EXPORT int POC_CALL poc_win64_compare_exchange(poc_atomic64 *p,
                                                   uint64_t *expected,
                                                   uint64_t desired) {
    return poc_atomic64_compare_exchange(p, expected, desired);
}
