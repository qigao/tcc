/* TCC native Android -c probe needs no cross-sysroot headers. */
int android_tcc_object(int x) { return (x ^ 0x1234) + 7; }
