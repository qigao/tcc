/* SPDX-License-Identifier: MIT */
#include "obfh_arm64_poc.h"
#include "obfh_arm64_fixture.h"

static uint32_t hash_bytes(const unsigned char *data, size_t count) {
    uint32_t hash = 0x811c9dc5u;
    size_t i;
    for (i = 0; i < count; ++i) {
        hash = (hash ^ data[i]) * 0x01000193u;
    }
    return hash;
}

int main(void) {
    unsigned char result[sizeof obfh_arm64_fixture_bytes];
    uint32_t n;
    int seen_true = 0;
    int seen_false = 0;
    obfh_arm64_decode_bytes(result, obfh_arm64_fixture_bytes, sizeof result);
    if (result[sizeof result - 1] != 0 ||
        hash_bytes(result, sizeof result) != OBFH_ARM64_FIXTURE_HASH)
        return 10;

    for (n = 0; n < 257u; ++n) {
        uint32_t left = n * 0x9e3779b9u;
        uint32_t right = (n + 19u) * 0x85ebca6bu;
        if (obfh_arm64_vm_add(left, right) != left + right) return 11;
        if (obfh_arm64_vm_xor(left, right) != (left ^ right)) return 12;
        if (obfh_arm64_branch_equal(left, left)) ++seen_true;
        if (!obfh_arm64_branch_equal(left, right)) ++seen_false;
    }
    if (seen_true != 257 || seen_false != 257) return 13;
    return 0;
}
