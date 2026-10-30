/* SPDX-License-Identifier: MIT
 * Differential with the native-C baseline of upstream tests/vm_branches.c.
 * Only explicit VM_IF/ELSE_IF/ELSE is currently supported; not auto-CFLOW.
 */
#include <stdint.h>
#include <stddef.h>
#include "obfh_arm64_vm_branches.h"
#if defined(VM_ADD) || defined(HIDE_STRING) || defined(BREAK_STACK_CFLOW)
#error "Unported upstream features may not be silently exported."
#endif

static int reference_chain(int a, double b, void *p, double q) {
    if (a) return 1;
    else if (b) return 2;
    else if (p) return 3;
    else if (q) return 4;
    else return 5;
}
static int virtual_chain(int a, double b, void *p, double q) {
    VM_IF(a) { return 1; }
    VM_ELSE_IF(b) { return 2; }
    VM_ELSE_IF(p) { return 3; }
    VM_ELSE_IF(q) { return 4; }
    VM_ELSE { return 5; }
    return -1;
}
static int reference_recursive(int n) {
    if (n <= 0) return 1;
    else if (n & 1) return n + reference_recursive(n - 1);
    else return reference_recursive(n - 1) - n;
}
static int virtual_recursive(int n) {
    VM_IF(n <= 0) { return 1; }
    VM_ELSE_IF(n & 1) { return n + virtual_recursive(n - 1); }
    VM_ELSE { return virtual_recursive(n - 1) - n; }
    return -1000;
}
static int exercise_conditions(void) {
    int effects = 0, selected = 0, sum = 0;
    int i, a, b;
    uint64_t large = UINT64_C(1) << 48;
    VM_IF(++effects == 1) { selected = 1; }
    VM_ELSE_IF(++effects == 2) { selected = 2; }
    VM_ELSE { selected = 3; }
    if (effects != 1 || selected != 1) return 11;
    effects = 0;
    VM_IF(++effects < 0) { selected = 1; }
    VM_ELSE_IF(++effects == 2) { selected = 2; }
    VM_ELSE { selected = 3; }
    if (effects != 2 || selected != 2) return 12;
    effects = 0;
    VM_IF(0 && ++effects) { selected = 1; }
    VM_ELSE_IF(1 || ++effects) { selected = 2; }
    VM_ELSE { selected = 3; }
    if (effects != 0 || selected != 2) return 13;
    effects = 0;
    VM_IF((++effects, 0.25)) selected = 7;
    if (effects != 1 || selected != 7) return 14;
    VM_IF(large) selected = 8;
    if (selected != 8) return 15;
    for (i = 0; i < 32; ++i) {
        VM_IF(i == 3) { continue; }
        VM_ELSE_IF(i == 9) { break; }
        VM_ELSE { sum += i; }
    }
    if (sum != 33) return 16;
    VM_IF(1) { goto reached; }
    return 17;
reached:
    for (a = 0; a < 2; ++a)
        for (b = 0; b < 2; ++b) {
            selected = 0;
            /* Braced outer scope retains the same closest-else binding without
             * triggering -Wdangling-else under GCC/Clang -Werror. */
            VM_IF(a) {
                VM_IF(b)
                    selected = 11;
                VM_ELSE selected = 12;
            }
            if (selected != (a ? (b ? 11 : 12) : 0)) return 18;
        }
    return 0;
}
int main(void) {
    union { uint64_t bits; double value; } nan_value, infinity;
    uint32_t random = 0x91745123u;
    int result = exercise_conditions();
    int selected = 0, i;
    if (result) return result;
    nan_value.bits = UINT64_C(0x7ff8000000000001);
    infinity.bits = UINT64_C(0x7ff0000000000000);
    for (i = 0; i < 1000; ++i) {
        int a;
        double b, q;
        void *p;
        random = random * 1664525u + 1013904223u;
        a = (random & 1u) ? 0 : -7;
        b = (random & 2u) ? -0.0 : ((random & 32u) ? infinity.value : 0.25);
        p = (random & 4u) ? (void *)0 : (void *)&selected;
        q = (random & 8u) ? nan_value.value : 0.0;
        if (virtual_chain(a, b, p, q) != reference_chain(a, b, p, q))
            return 31;
        if (virtual_chain(0, 0.0, (void *)0, 0.0) != 5) return 32;
    }
    for (i = 0; i < 32; ++i)
        if (virtual_recursive(i) != reference_recursive(i)) return 33;
    return 0;
}
