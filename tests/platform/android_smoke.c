/* Bionic native runtime check. Clang reference, not proof of TCC codegen.
 * The TinyCC ELF is separately built by NDK and then executed on Android. */
#if !defined(__ANDROID__)
#error "Android NDK/Bionic is required for this executable"
#endif
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <dlfcn.h>
#include <stdint.h>
static int value;
static void *worker(void *arg) {
    value = 37 + *(int *)arg;
    return arg;
}
int main(void) {
    pthread_t thread;
    int arg = 5;
    void *result = NULL;
    struct timespec ts;
    void *libc;
    if (pthread_create(&thread, NULL, worker, &arg) ||
        pthread_join(thread, &result) ||
        result != &arg || value != 42) return 1;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) || ts.tv_sec <= 0) return 2;
    libc = dlopen("libc.so", RTLD_NOW);
    if (!libc) return 3;
    if (!dlsym(libc, "malloc") || dlclose(libc)) return 4;
    puts("android: native Bionic pthread, clock and dlopen OK");
    return 0;
}
