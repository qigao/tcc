#ifndef TCC_POC_THREADS_H
#define TCC_POC_THREADS_H
/* Explicitly limited C11-style test facade, not the C11 <threads.h> ABI.
 * Private structures MUST NOT cross a compiler/DLL boundary or be moved while
 * live. The caller owns storage until join/destroy completes. */
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef int (*poc_thread_fn)(void *);
typedef void (*poc_tss_destructor)(void *);
typedef struct poc_thread {
#ifdef _WIN32
    HANDLE native;
#else
    pthread_t native;
#endif
    poc_thread_fn fn;
    void *argument;
    int result;
    int active;
} poc_thread;
#define POC_THREAD_INIT {0}

typedef struct poc_mutex {
#ifdef _WIN32
    CRITICAL_SECTION native;
#else
    pthread_mutex_t native;
#endif
    int active;
} poc_mutex;

typedef struct poc_condition {
#ifdef _WIN32
    CONDITION_VARIABLE native;
#else
    pthread_cond_t native;
#endif
    int active;
} poc_condition;

typedef struct poc_tss {
#ifdef _WIN32
    DWORD native;
#else
    pthread_key_t native;
#endif
    int active;
} poc_tss;

/* 0 success, -1 invalid input/state or native failure. No implicit fallback.
 * Caller must initialize all objects with zero-filled storage first.
 * Destroying a mutex/condition with waiters, concurrent operations or live
 * threads is forbidden. Join must not be invoked by its own thread. */
int poc_thread_create(poc_thread *t, poc_thread_fn fn, void *argument);
int poc_thread_join(poc_thread *t, int *result);
int poc_mutex_init(poc_mutex *m);
int poc_mutex_lock(poc_mutex *m);
int poc_mutex_unlock(poc_mutex *m);
int poc_mutex_destroy(poc_mutex *m);
int poc_condition_init(poc_condition *c);
int poc_condition_wait(poc_condition *c, poc_mutex *m);
int poc_condition_signal(poc_condition *c);
int poc_condition_broadcast(poc_condition *c);
int poc_condition_destroy(poc_condition *c);
int poc_tss_create(poc_tss *key, poc_tss_destructor destructor);
int poc_tss_set(poc_tss *key, void *value);
void *poc_tss_get(poc_tss *key);
int poc_tss_destroy(poc_tss *key);
#ifdef __cplusplus
}
#endif
#endif
