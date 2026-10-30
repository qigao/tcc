/* SPDX-License-Identifier: MIT */
#include <stdlib.h>
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

/* Ordinary C control/return flow serves as an independent semantic oracle. */
static uint32_t baseline_flow(uint32_t input, uint32_t *visits) {
    uint32_t total = 0u;
    uint32_t i;
    for (i = 0u; i < 13u; ++i) {
        ++*visits;
        if (i == 2u) continue;
        if (i == 10u) break;
        switch ((input + i) % 4u) {
        case 0u: total += i; break;
        case 1u: total ^= i + 3u; break;
        case 2u: total += 0xfffffff0u; break;
        default: total += 17u; break;
        }
    }
    if (input == 0u) return total + 5u;
    if (input == UINT32_MAX) return total ^ 0xa5a55a5au;
    return total ^ (input * 0x9e3779b9u);
}

static uint32_t protected_flow(uint32_t input, uint32_t *visits) {
    uint32_t total = 0u;
    uint32_t i;
    for (i = 0u; i < 13u; ++i) {
        ++*visits;
        if (obfh_arm64_branch_equal(i, 2u)) continue;
        if (obfh_arm64_branch_equal(i, 10u)) break;
        switch ((input + i) % 4u) {
        case 0u: total = obfh_arm64_vm_add(total, i); break;
        case 1u: total = obfh_arm64_vm_xor(total, i + 3u); break;
        case 2u: total = obfh_arm64_vm_add(total, 0xfffffff0u); break;
        default: total = obfh_arm64_vm_add(total, 17u); break;
        }
    }
    if (obfh_arm64_branch_equal(input, 0u)) return obfh_arm64_vm_add(total, 5u);
    if (obfh_arm64_branch_equal(input, UINT32_MAX))
        return obfh_arm64_vm_xor(total, 0xa5a55a5au);
    return obfh_arm64_vm_xor(total, input * 0x9e3779b9u);
}

static int pointer_roundtrips(void) {
    static uint32_t shared = 0xabcddcbau;
    uint32_t local = 0x12345678u;
    unsigned char buffer[16] = {0};
    unsigned char *heap = (unsigned char *)malloc(32u);
    const void *objects[5];
    size_t i;
    if (!heap) return 20;
    objects[0] = (const void *)0;
    objects[1] = &local;
    objects[2] = &shared;
    objects[3] = buffer;
    objects[4] = heap;
    for (i = 0; i < 5; ++i) {
        volatile uintptr_t slot = obfh_arm64_pointer_encode(objects[i]);
        const void *restored = obfh_arm64_pointer_decode(slot);
        if (restored != objects[i] || obfh_arm64_pointer_encode(restored) != slot) {
            free(heap);
            return 21;
        }
    }
    *(uint32_t *)obfh_arm64_pointer_decode(obfh_arm64_pointer_encode(&local)) = 37u;
    *(uint32_t *)obfh_arm64_pointer_decode(obfh_arm64_pointer_encode(&shared)) = 53u;
    ((unsigned char *)obfh_arm64_pointer_decode(obfh_arm64_pointer_encode(buffer)))[0] = 11u;
    ((unsigned char *)obfh_arm64_pointer_decode(obfh_arm64_pointer_encode(heap)))[0] = 23u;
    if (local != 37u || shared != 53u || buffer[0] != 11u || heap[0] != 23u) {
        free(heap);
        return 22;
    }
    free(heap);
    return 0;
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
    for (n = 0u; n < 1025u; ++n) {
        uint32_t normal_visits = 0u, protected_visits = 0u;
        uint32_t input = (n == 1024u) ? UINT32_MAX : n;
        uint32_t normal = baseline_flow(input, &normal_visits);
        uint32_t transformed = protected_flow(input, &protected_visits);
        if (normal != transformed || normal_visits != protected_visits || normal_visits != 11u)
            return 14;
    }
    return pointer_roundtrips();
}
