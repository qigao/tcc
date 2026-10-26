/* NDK-built caller linked with an Android-device TinyCC-generated object. */
#include <stdio.h>
extern int android_tcc_object(int);
int main(void) {
    if (android_tcc_object(0x1234) != 7 ||
        android_tcc_object(0x0000) != (0x1234 + 7)) return 1;
    puts("Android: TinyCC native ELF object interoperated with NDK Clang PASS");
    return 0;
}
