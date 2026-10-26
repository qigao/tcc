/* Darwin runtime gate: ELF object-file compatibility is not implied. */
/* Do not assume TinyCC defines Clang's full Darwin macro set:
 * native Mach-O format, architecture and execution are checked by CI. */
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <dlfcn.h>
#include <time.h>
static int done;
static void *worker(void *data) {
    int number = *(int *)data;
    done = number * 7;
    return data;
}
int main(void) {
    pthread_t thread;
    void *result = NULL;
    int number = 11;
    struct timespec ts;
    void *system;
    if (pthread_create(&thread, NULL, worker, &number) != 0 ||
        pthread_join(thread, &result) != 0 || result != &number ||
        done != 77) return 1;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0 || ts.tv_sec <= 0) return 2;
    system = dlopen("/usr/lib/libSystem.B.dylib", RTLD_LAZY);
    if (!system) return 3;
    if (!dlsym(system, "malloc") || dlclose(system) != 0) return 4;
    puts("macOS: native TinyCC Mach-O, pthread, libSystem and clock PASS");
    return 0;
}
