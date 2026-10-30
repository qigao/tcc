/* SPDX-License-Identifier: MIT */
#include <stdlib.h>
#include "obfh_arm64_poc.h"
#include "obfh_arm64_upstream_hide_string.h"
#include "obfh_arm64_fixture.h"
#include <string.h>

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

/* Original HIDE_STRING returns a pointer ONE byte past a leading NUL
 * inside the caller's block-duration STACK_STRING compound literal. The
 * C compiler receives only ciphertext arrays generated before compilation.
 * These exact fixtures prove semantic lifetime, NOT generic literal macro
 * protection or cryptographic strength.
 */
_Static_assert(OBFH_ARM64_FIXTURE_UTF8_COUNT==25u &&
               OBFH_ARM64_FIXTURE_UTF8_HASH==UINT32_C(0xcc6f4208),
               "Independent UTF-8 fixture length/hash mismatch");
_Static_assert(OBFH_ARM64_FIXTURE_EMBEDDED_COUNT==13u &&
               OBFH_ARM64_FIXTURE_EMBEDDED_HASH==UINT32_C(0x9d330501),
               "Independent embedded-NUL fixture length/hash mismatch");
_Static_assert(OBFH_ARM64_FIXTURE_EMPTY_COUNT==1u &&
               OBFH_ARM64_FIXTURE_EMPTY_HASH==UINT32_C(0x050c5d1f),
               "Independent empty fixture length/hash mismatch");
_Static_assert(OBFH_ARM64_FIXTURE_COUNT==17u &&
               OBFH_ARM64_FIXTURE_HASH==UINT32_C(0xae597b0f),
               "Independent original default fixture hash mismatch");

static int hidden_stack_contract(void) {
    const size_t count=sizeof obfh_arm64_fixture_bytes;
    const size_t utf8_count=sizeof obfh_arm64_fixture_utf8_bytes;
    const size_t embedded_count=sizeof obfh_arm64_fixture_embedded_bytes;
    char *const first=OBFH_HIDE_ENCODED_SITE(
        obfh_arm64_fixture_bytes,OBFH_ARM64_FIXTURE_HASH);
    char *const second=OBFH_HIDE_ENCODED_SITE(
        obfh_arm64_fixture_bytes,OBFH_ARM64_FIXTURE_HASH);
    char *const utf8=OBFH_HIDE_ENCODED_SITE(
        obfh_arm64_fixture_utf8_bytes,OBFH_ARM64_FIXTURE_UTF8_HASH);
    char *const embedded=OBFH_HIDE_ENCODED_SITE(
        obfh_arm64_fixture_embedded_bytes,OBFH_ARM64_FIXTURE_EMBEDDED_HASH);
    char *const empty=OBFH_HIDE_ENCODED_SITE(
        obfh_arm64_fixture_empty_bytes,OBFH_ARM64_FIXTURE_EMPTY_HASH);
    unsigned char corrupt[sizeof obfh_arm64_fixture_bytes];
    char frame[sizeof obfh_arm64_fixture_bytes+2u];
    char untouched[sizeof frame];
    size_t i;

    if(!first || !second || !utf8 || !embedded || !empty ||
       first==second || first==utf8 || second==embedded) return 50;
    if(first[-1]!='\0' || first[count-1u]!='\0' || first[count]!='\0' ||
       second[-1]!='\0' || second[count-1u]!='\0' ||
       hash_bytes((const unsigned char *)first,count)!=OBFH_ARM64_FIXTURE_HASH ||
       hash_bytes((const unsigned char *)second,count)!=OBFH_ARM64_FIXTURE_HASH)
        return 51;
    /* Byte-length (not strlen) is authoritative for CJK/emoji and NUL. */
    if(utf8[-1]!='\0' || utf8[utf8_count-1u]!='\0' ||
       (unsigned char)utf8[0]!=0xe6u ||
       hash_bytes((const unsigned char *)utf8,utf8_count)!=
            OBFH_ARM64_FIXTURE_UTF8_HASH)
        return 52;
    if(embedded[-1]!='\0' || embedded[5]!='\0' ||
       embedded[embedded_count-1u]!='\0' ||
       (unsigned char)embedded[6]!=0x73u ||
       hash_bytes((const unsigned char *)embedded,embedded_count)!=
            OBFH_ARM64_FIXTURE_EMBEDDED_HASH)
        return 53;
    if(empty[-1]!='\0' || empty[0]!='\0' || empty[1]!='\0' ||
       hash_bytes((const unsigned char *)empty,1u)!=
            OBFH_ARM64_FIXTURE_EMPTY_HASH)
        return 54;
    /* Separate lexical compound literal sites MUST be distinct borrow
     * targets. Mutating one cannot modify another caller-owned buffer. */
    first[0]^=1;
    if(hash_bytes((const unsigned char *)second,count)!=
       OBFH_ARM64_FIXTURE_HASH) return 55;
    obfh_arm64_hidden_clear(first,count);
    for(i=0u;i<count;++i)if(first[i]!=0) return 56;
    if(hash_bytes((const unsigned char *)second,count)!=
       OBFH_ARM64_FIXTURE_HASH) return 57;

    /* Failure after decoding must wipe all caller-owned frame bytes. */
    memcpy(corrupt,obfh_arm64_fixture_bytes,sizeof corrupt);
    corrupt[2]^=0x5au;
    memset(frame,0x5c,sizeof frame);
    if(obfh_arm64_hidden_decode_stack(frame,sizeof frame,corrupt,
          sizeof corrupt,OBFH_ARM64_FIXTURE_HASH)!=NULL) return 58;
    for(i=0u;i<sizeof frame;++i)if(frame[i]!=0) return 59;
    /* Invalid admission must reject before writing any output. */
    memset(frame,0x5c,sizeof frame);
    memcpy(untouched,frame,sizeof frame);
    if(obfh_arm64_hidden_decode_stack(frame,sizeof frame,
          (const unsigned char *)0,count,OBFH_ARM64_FIXTURE_HASH)!=NULL ||
       memcmp(frame,untouched,sizeof frame)!=0) return 60;
    if(obfh_arm64_hidden_decode_stack(frame,sizeof frame-1u,
          obfh_arm64_fixture_bytes,count,OBFH_ARM64_FIXTURE_HASH)!=NULL ||
       memcmp(frame,untouched,sizeof frame)!=0) return 61;
    if(obfh_arm64_hidden_decode_stack(frame,sizeof frame,
          obfh_arm64_fixture_bytes,0u,OBFH_ARM64_FIXTURE_HASH)!=NULL ||
       memcmp(frame,untouched,sizeof frame)!=0) return 62;
    /* Caller-initiated scrub can also remove UTF-8 and NUL data. */
    obfh_arm64_hidden_clear(utf8,utf8_count);
    obfh_arm64_hidden_clear(embedded,embedded_count);
    for(i=0u;i<utf8_count;++i)if(utf8[i]!=0) return 63;
    for(i=0u;i<embedded_count;++i)if(embedded[i]!=0) return 64;
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
    {
        int hidden=hidden_stack_contract();
        if(hidden) return hidden;
    }
    return pointer_roundtrips();
}
