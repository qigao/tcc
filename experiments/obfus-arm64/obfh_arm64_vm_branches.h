/* SPDX-License-Identifier: MIT
 * Explicit VM_IF/VM_ELSE_IF/VM_ELSE source-signature feasibility on AArch64.
 * Does NOT implement the upstream encoded VM dispatcher or auto-keyword hooks.
 */
#ifndef QIGAO_OBFH_ARM64_VM_BRANCHES_H
#define QIGAO_OBFH_ARM64_VM_BRANCHES_H
#include "obfh_arm64_poc.h"

#if defined(VM_IF) || defined(VM_ELSE_IF) || defined(VM_ELSE)
#error "Existing VM branch macros conflict with the experimental ARM64 subset."
#endif

/* Boolean normalization in the *caller's* expression preserves long double,
 * pointer, signed-zero and NaN truth semantics before the A64 value transport.
 * No conditional expression is evaluated more than once. */
static inline int obfh_arm64_vm_truth(int normalized_truth) {
    return obfh_arm64_branch_equal((uint32_t)(normalized_truth != 0), 1u);
}

/* Keep the upstream region-14 'else if (true)' chaining contract so dangling
 * else binds to the same nearest conditional as in the original API. */
#define VM_IF(condition) if (obfh_arm64_vm_truth(!!(condition)))
#define VM_ELSE_IF(condition) else if (obfh_arm64_vm_truth(!!(condition)))
#define VM_ELSE else if (obfh_arm64_vm_truth(1))

#endif
