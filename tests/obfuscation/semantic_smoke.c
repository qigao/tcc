#include <stdio.h>
#include <string.h>
#if defined(POC_ENABLE_OBFUS_H)
#if !defined(_WIN32) || !defined(__TINYC__)
#error "obfus.h mode is only accepted on Windows TinyCC."
#endif
/* Test only non-evasive obfuscation; no anti-debugging or fake signatures. */
#define OBFH_DISABLE_ANTIDEBUG 1
#define OBFH_DISABLE_PDATA_DECOYS 1
#define OBFH_FAKE_SIGNATURES 0
#include "obfus.h"
#endif
int main(void) {
#if defined(POC_ENABLE_OBFUS_H)
    const char *value = HIDE_STRING("tcc-poc");
    int arithmetic = (int)VM_ADD(5, 7);
#else
    const char *value = "tcc-poc";
    int arithmetic = 5 + 7;
#endif
    if (strcmp(value, "tcc-poc") || arithmetic != 12) return 1;
    puts("obfuscation semantic parity: tcc-poc 12");
    return 0;
}
