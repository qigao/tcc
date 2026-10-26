#include "poc/threads.h"
#include <stddef.h>

#ifdef _WIN32
static DWORD WINAPI poc_thread_entry(LPVOID arg) {
    poc_thread *t = (poc_thread *)arg;
    t->result = t->fn(t->argument);
    return 0;
}
#else
static void *poc_thread_entry(void *arg) {
    poc_thread *t = (poc_thread *)arg;
    t->result = t->fn(t->argument);
    return NULL;
}
#endif

int poc_thread_create(poc_thread *t, poc_thread_fn fn, void *argument) {
    if (!t || !fn || t->active) return -1;
    t->fn = fn;
    t->argument = argument;
    t->result = 0;
#ifdef _WIN32
    t->native = CreateThread(NULL, 0, poc_thread_entry, t, 0, NULL);
    if (!t->native) return -1;
#else
    if (pthread_create(&t->native, NULL, poc_thread_entry, t) != 0) return -1;
#endif
    t->active = 1;
    return 0;
}

int poc_thread_join(poc_thread *t, int *result) {
    if (!t || !t->active) return -1;
#ifdef _WIN32
    if (GetCurrentThreadId() == GetThreadId(t->native)) return -1;
    if (WaitForSingleObject(t->native, INFINITE) != WAIT_OBJECT_0) return -1;
    if (!CloseHandle(t->native)) return -1;
    t->native = NULL;
#else
    if (pthread_equal(pthread_self(), t->native) || pthread_join(t->native, NULL) != 0) return -1;
#endif
    t->active = 0;
    if (result) *result = t->result;
    return 0;
}

int poc_mutex_init(poc_mutex *m) {
    if (!m || m->active) return -1;
#ifdef _WIN32
    if (!InitializeCriticalSectionEx(&m->native, 0, 0)) return -1;
#else
    if (pthread_mutex_init(&m->native, NULL) != 0) return -1;
#endif
    m->active = 1;
    return 0;
}
int poc_mutex_lock(poc_mutex *m) {
    if (!m || !m->active) return -1;
#ifdef _WIN32
    EnterCriticalSection(&m->native);
    return 0;
#else
    return pthread_mutex_lock(&m->native) == 0 ? 0 : -1;
#endif
}
int poc_mutex_unlock(poc_mutex *m) {
    if (!m || !m->active) return -1;
#ifdef _WIN32
    LeaveCriticalSection(&m->native);
    return 0;
#else
    return pthread_mutex_unlock(&m->native) == 0 ? 0 : -1;
#endif
}
int poc_mutex_destroy(poc_mutex *m) {
    if (!m || !m->active) return -1;
#ifdef _WIN32
    DeleteCriticalSection(&m->native);
#else
    if (pthread_mutex_destroy(&m->native) != 0) return -1;
#endif
    m->active = 0;
    return 0;
}
int poc_condition_init(poc_condition *c) {
    if (!c || c->active) return -1;
#ifdef _WIN32
    /* Use the documented pointer-sized Windows condition variable layout.
     * Resolve system APIs dynamically to support TinyCC SDKs that omit the
     * CONDITION_VARIABLE typedef and these newer import declarations. */
    c->native = NULL;
    if (!GetProcAddress(GetModuleHandleA("kernel32.dll"), "SleepConditionVariableCS") ||
        !GetProcAddress(GetModuleHandleA("kernel32.dll"), "WakeConditionVariable") ||
        !GetProcAddress(GetModuleHandleA("kernel32.dll"), "WakeAllConditionVariable")) return -1;
#else
    if (pthread_cond_init(&c->native, NULL) != 0) return -1;
#endif
    c->active = 1;
    return 0;
}
int poc_condition_wait(poc_condition *c, poc_mutex *m) {
    if (!c || !c->active || !m || !m->active) return -1;
#ifdef _WIN32
    typedef BOOL (WINAPI *sleep_fn)(void *, CRITICAL_SECTION *, DWORD);
    HMODULE kernel = GetModuleHandleA("kernel32.dll");
    sleep_fn fn = kernel ? (sleep_fn)GetProcAddress(kernel, "SleepConditionVariableCS") : NULL;
    return fn && fn(&c->native, &m->native, INFINITE) ? 0 : -1;
#else
    return pthread_cond_wait(&c->native, &m->native) == 0 ? 0 : -1;
#endif
}
int poc_condition_signal(poc_condition *c) {
    if (!c || !c->active) return -1;
#ifdef _WIN32
    typedef void (WINAPI *wake_fn)(void *);
    HMODULE kernel = GetModuleHandleA("kernel32.dll");
    wake_fn fn = kernel ? (wake_fn)GetProcAddress(kernel, "WakeConditionVariable") : NULL;
    if (!fn) return -1;
    fn(&c->native);
    return 0;
#else
    return pthread_cond_signal(&c->native) == 0 ? 0 : -1;
#endif
}
int poc_condition_broadcast(poc_condition *c) {
    if (!c || !c->active) return -1;
#ifdef _WIN32
    typedef void (WINAPI *wake_fn)(void *);
    HMODULE kernel = GetModuleHandleA("kernel32.dll");
    wake_fn fn = kernel ? (wake_fn)GetProcAddress(kernel, "WakeAllConditionVariable") : NULL;
    if (!fn) return -1;
    fn(&c->native);
    return 0;
#else
    return pthread_cond_broadcast(&c->native) == 0 ? 0 : -1;
#endif
}
int poc_condition_destroy(poc_condition *c) {
    if (!c || !c->active) return -1;
#ifndef _WIN32
    if (pthread_cond_destroy(&c->native) != 0) return -1;
#endif
    c->active = 0;
    return 0;
}
int poc_tss_create(poc_tss *key, poc_tss_destructor destructor) {
    if (!key || key->active) return -1;
#ifdef _WIN32
    key->native = FlsAlloc((PFLS_CALLBACK_FUNCTION)destructor);
    if (key->native == FLS_OUT_OF_INDEXES) return -1;
#else
    if (pthread_key_create(&key->native, destructor) != 0) return -1;
#endif
    key->active = 1;
    return 0;
}
int poc_tss_set(poc_tss *key, void *value) {
    if (!key || !key->active) return -1;
#ifdef _WIN32
    return FlsSetValue(key->native, value) ? 0 : -1;
#else
    return pthread_setspecific(key->native, value) == 0 ? 0 : -1;
#endif
}
void *poc_tss_get(poc_tss *key) {
    if (!key || !key->active) return NULL;
#ifdef _WIN32
    return FlsGetValue(key->native);
#else
    return pthread_getspecific(key->native);
#endif
}
int poc_tss_destroy(poc_tss *key) {
    if (!key || !key->active) return -1;
#ifdef _WIN32
    if (!FlsFree(key->native)) return -1;
#else
    if (pthread_key_delete(key->native) != 0) return -1;
#endif
    key->active = 0;
    return 0;
}
