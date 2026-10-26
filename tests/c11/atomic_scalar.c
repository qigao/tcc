/* ISO C11 source-level contract: compile with the compiler's own stdatomic.h.
 * Never reinterpret poc_atomic_* storage as a native _Atomic object.
 */
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __STDC_NO_ATOMICS__
#error "Compiler reports that C11 atomics are unavailable"
#endif

static atomic_flag flag = ATOMIC_FLAG_INIT;
static atomic_uint word = ATOMIC_VAR_INIT(0u);
static atomic_ullong wide = ATOMIC_VAR_INIT(0ull);
static _Atomic(void *) pointer = ATOMIC_VAR_INIT(NULL);
static struct {
    unsigned char before;
    atomic_uchar byte;
    unsigned char middle;
    atomic_ushort half;
    unsigned char after;
} sentinels;

int main(void) {
    unsigned expected;
    unsigned long long expected_wide;
    void *expected_ptr;
    int first = 11, second = 22;

    atomic_init(&word, 0u);
    atomic_store_explicit(&word, 7u, memory_order_release);
    if (atomic_load_explicit(&word, memory_order_acquire) != 7u) return 1;
    expected = 8u;
    if (atomic_compare_exchange_strong_explicit(&word, &expected, 9u,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire) || expected != 7u)
        return 2;
    if (!atomic_compare_exchange_strong(&word, &expected, 9u) || expected != 7u)
        return 3;
    if (atomic_exchange(&word, 3u) != 9u) return 4;
    if (atomic_fetch_add_explicit(&word, 5u, memory_order_relaxed) != 3u) return 5;
    if (atomic_fetch_sub(&word, 3u) != 8u) return 6;
    if (atomic_fetch_or(&word, 0x10u) != 5u) return 7;
    if (atomic_fetch_and(&word, 0x17u) != 0x15u) return 8;
    if (atomic_fetch_xor(&word, 0x06u) != 0x15u) return 9;
    if (atomic_load(&word) != 0x13u) return 10;
    expected = 0x12u;
    if (atomic_compare_exchange_weak_explicit(&word, &expected, 1u,
                                               memory_order_acquire,
                                               memory_order_relaxed) || expected != 0x13u)
        return 11;
    do { expected = 0x13u; }
    while (!atomic_compare_exchange_weak(&word, &expected, 42u));
    if (atomic_load(&word) != 42u) return 12;

    sentinels.before = 0xa5u;
    sentinels.middle = 0x5au;
    sentinels.after = 0x3cu;
    atomic_init(&sentinels.byte, 255u);
    if (atomic_fetch_add(&sentinels.byte, 1u) != 255u ||
        atomic_load(&sentinels.byte) != 0u) return 13;
    atomic_init(&sentinels.half, 65535u);
    if (atomic_fetch_add(&sentinels.half, 1u) != 65535u ||
        atomic_load(&sentinels.half) != 0u) return 14;
    if (sentinels.before != 0xa5u || sentinels.middle != 0x5au ||
        sentinels.after != 0x3cu) return 15;

    atomic_init(&wide, 0x100000000ull);
    expected_wide = 7ull;
    if (atomic_compare_exchange_strong(&wide, &expected_wide, 12ull) ||
        expected_wide != 0x100000000ull) return 16;
    if (!atomic_compare_exchange_strong(&wide, &expected_wide, 0x100000007ull))
        return 17;
    if (atomic_fetch_add(&wide, 3ull) != 0x100000007ull ||
        atomic_load(&wide) != 0x10000000aull) return 18;

    atomic_store(&pointer, &first);
    expected_ptr = &second;
    if (atomic_compare_exchange_strong(&pointer, &expected_ptr, &second) ||
        expected_ptr != &first) return 19;
    if (!atomic_compare_exchange_strong(&pointer, &expected_ptr, &second) ||
        atomic_load(&pointer) != &second) return 20;

    if (atomic_flag_test_and_set_explicit(&flag, memory_order_acquire)) return 21;
    if (!atomic_flag_test_and_set(&flag)) return 22;
    atomic_flag_clear_explicit(&flag, memory_order_release);
    if (atomic_flag_test_and_set(&flag)) return 23;
    atomic_flag_clear(&flag);

    atomic_thread_fence(memory_order_seq_cst);
    atomic_signal_fence(memory_order_acq_rel);
    /* Lock-free is not guaranteed; a false result is not a test failure. */
    (void)atomic_is_lock_free(&word);
    puts("C11 atomic scalar: 8/16/32/64 pointer CAS/RMW, flag, fences PASS");
    return 0;
}
