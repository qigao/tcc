/* This is a capability-denial test, NOT stdatomic conformance.
 * The pinned Windows and Darwin TinyCC toolchains report NO_ATOMICS.
 * Fail if support appears; then promote the real positive tests instead.
 */
#include <stdatomic.h>
#include <stdio.h>

#ifndef __TINYC__
#error "Capability denial must be compiled by TinyCC"
#endif
#if !defined(_WIN32) && !defined(__APPLE__)
#error "Capability denial must run on Windows or Darwin"
#endif
#ifndef __STDC_NO_ATOMICS__
#error "Atomic capability changed: replace denial with the positive C11 tests"
#endif
#if __STDC_NO_ATOMICS__ != 1
#error "Expected a true __STDC_NO_ATOMICS__ capability declaration"
#endif

int main(void) {
    puts("TinyCC explicitly reports C11 atomics unavailable: NOT QUALIFIED");
    return 0;
}
