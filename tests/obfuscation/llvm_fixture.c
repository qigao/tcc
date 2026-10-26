#include <stdio.h>
#include <stdlib.h>
/* Deliberately nontrivial CFG for real LLVM flattening; no secrets or evasion. */
static int transform(int x) {
    unsigned state = (unsigned)x;
    unsigned i;
    for (i = 0; i < 19; ++i) {
        if ((state & 3u) == 0u) {
            state = (state << 1) ^ (0x9e3779b9u + i);
        } else if ((state & 3u) == 1u) {
            state = (state >> 1) + 17u * i;
        } else {
            state = state ^ (state << 3) ^ (11u + i);
        }
    }
    return (int)(state & 0xffffu);
}
int main(int argc, char **argv) {
    int value;
    if (argc != 2) return 2;
    value = atoi(argv[1]);
    printf("llvm-poc:%d:%d\n", value, transform(value));
    return 0;
}
