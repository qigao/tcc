#include "poc/atomic64.h"
#if UINTPTR_MAX != UINT64_MAX
#error "Pointer CAS is only supported on flat native 64-bit targets"
#endif
typedef char poc_atomic64_layout_guard[(sizeof(poc_atomic64) == 8) ? 1 : -1];
typedef char poc_atomic_ptr_layout_guard[(sizeof(poc_atomic_ptr) == 8) ? 1 : -1];
static uint64_t encode_pointer(const void *p) {
    return (uint64_t)(uintptr_t)p;
}
static void *decode_pointer(uint64_t v) {
    return (void *)(uintptr_t)v;
}
void *poc_atomic_ptr_load(const poc_atomic_ptr *p) {
    return decode_pointer(poc_atomic64_load(&p->bits));
}
void poc_atomic_ptr_store(poc_atomic_ptr *p, void *v) {
    poc_atomic64_store(&p->bits, encode_pointer(v));
}
void *poc_atomic_ptr_exchange(poc_atomic_ptr *p, void *v) {
    return decode_pointer(poc_atomic64_exchange(&p->bits, encode_pointer(v)));
}
int poc_atomic_ptr_compare_exchange(poc_atomic_ptr *p, void **expected, void *desired) {
    uint64_t observed;
    int ok;
    if (!expected) return 0;
    observed = encode_pointer(*expected);
    ok = poc_atomic64_compare_exchange(&p->bits, &observed, encode_pointer(desired));
    *expected = decode_pointer(observed);
    return ok;
}
