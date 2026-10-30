/* SPDX-License-Identifier: MIT
 * Source-faithful ARM64 HIDE_STRING caller-stack transport (encoded fixtures).
 * Upstream obfus.h blob 4c06fbc0:
 *   STACK_STRING("\0" str "\0") -> obfh_process_hidden_string(...) -> ptr+1.
 *
 * IMPORTANT: original HIDE_STRING(str) does NOT itself prove binary-string
 * encryption; LLVM StringObfuscation globally decodes from constructors and
 * has DIFFERENT object/lifetime semantics. This separate macro accepts only
 * build-time encoded arrays. It is NOT a drop-in for arbitrary literals,
 * does not implement rdtsc/junk/anti-debug side effects and is not a cipher.
 * No plaintext literal or runtime fallback is accepted by this API.
 */
#ifndef QIGAO_OBFH_ARM64_UPSTREAM_HIDE_STRING_H
#define QIGAO_OBFH_ARM64_UPSTREAM_HIDE_STRING_H
#include <stddef.h>
#include <stdint.h>
#include "obfh_arm64_poc.h"

#define OBFH_HIDE_MAX_ENCODED_BYTES 1024u

/* Caller explicitly owns the decoded C string until the enclosing C block
 * terminates (C99 compound literal lifetime). Never return this pointer
 * from a callee or retain it after the caller's scope ends.
 * A failed/corrupt packet is erased using volatile stores before returning
 * NULL: no fallback to source literal, heap or global decode buffer.
 */
static inline char *
obfh_arm64_hidden_decode_stack(char *frame,size_t frame_count,
                               const unsigned char *encoded,size_t count,
                               uint32_t expected_hash) {
    uint32_t hash=UINT32_C(0x811c9dc5);
    size_t i;
    if (!frame || !encoded || count<1u ||
        count>OBFH_HIDE_MAX_ENCODED_BYTES ||
        frame_count!=count+2u) return (char *)0;
    frame[0]='\0';
    frame[count+1u]='\0';
    obfh_arm64_decode_bytes((unsigned char *)(frame+1u),encoded,count);
    for(i=0u;i<count;++i)
        hash=(hash^(unsigned char)frame[1u+i])*UINT32_C(0x01000193);
    if(frame[count]!='\0' || hash!=expected_hash) {
        volatile unsigned char *const p=(volatile unsigned char *)frame;
        for(i=0u;i<frame_count;++i) p[i]=0u;
        return (char *)0;
    }
    /* Upstream returns one byte past the leading NUL, with a second trailing
     * sentinel after the decoded string's actual terminating NUL. */
    return frame+1u;
}

/* The array operand must be a compile-time emitted unsigned char ARRAY,
 * not a string literal or runtime pointer. sizeof is computed at the
 * lexical caller site to allocate an exact C block-duration compound array.
 * The macro does not capture or borrow a mutable global output buffer.
 */
#define OBFH_HIDE_ENCODED_SITE(ARRAY,RECEIPT) \
    obfh_arm64_hidden_decode_stack( \
        (char[sizeof(ARRAY)+2u]){0},sizeof(ARRAY)+2u, \
        (ARRAY),sizeof(ARRAY),(uint32_t)(RECEIPT))

/* Explicit lifetime-limited scrub for caller-owned output only. The original
 * HIDE_STRING does not automatically zero its stack buffer; neither can this
 * compiler-level experiment guarantee stack-frame erasure on every exit.
 */
static inline void obfh_arm64_hidden_clear(char *plain,size_t count) {
    volatile unsigned char *p=(volatile unsigned char *)plain;
    size_t i;
    if (!plain) return;
    for(i=0u;i<count;++i) p[i]=0u;
}
#endif
