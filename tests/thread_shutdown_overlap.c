#include "poc/threads.h"
#include <stdio.h>

typedef struct bounded_owner {
    poc_mutex mutex;
    poc_condition nonempty;
    poc_condition nonfull;
    poc_condition admitted;
    int present;
    int closed;
    int waiting_writer;
    int waiting_reader;
    int accepted_writes;
    int consumed_reads;
} bounded_owner;
static bounded_owner q;

static int writer(void *ignored) {
    int outcome = 0;
    (void)ignored;
    if (poc_mutex_lock(&q.mutex)) return -20;
    q.waiting_writer = 1;
    poc_condition_signal(&q.admitted);
    while (q.present && !q.closed)
        if (poc_condition_wait(&q.nonfull, &q.mutex)) { outcome = -21; break; }
    q.waiting_writer = 0;
    if (!outcome && q.closed) outcome = 10;
    else if (!outcome) { q.present = 1; ++q.accepted_writes; }
    poc_mutex_unlock(&q.mutex);
    return outcome;
}
static int reader(void *ignored) {
    int outcome = 0;
    (void)ignored;
    if (poc_mutex_lock(&q.mutex)) return -30;
    q.waiting_reader = 1;
    poc_condition_signal(&q.admitted);
    while (!q.present && !q.closed)
        if (poc_condition_wait(&q.nonempty, &q.mutex)) { outcome = -31; break; }
    q.waiting_reader = 0;
    if (!outcome && q.closed && !q.present) outcome = 20;
    else if (!outcome) { q.present = 0; ++q.consumed_reads; }
    poc_mutex_unlock(&q.mutex);
    return outcome;
}
static int close_owner(void) {
    if (poc_mutex_lock(&q.mutex)) return -1;
    q.closed = 1;
    poc_condition_broadcast(&q.nonfull);
    poc_condition_broadcast(&q.nonempty);
    return poc_mutex_unlock(&q.mutex);
}
int main(void) {
    poc_thread t = POC_THREAD_INIT;
    int result = -1;
    if (poc_mutex_init(&q.mutex) || poc_condition_init(&q.nonfull) ||
        poc_condition_init(&q.nonempty) || poc_condition_init(&q.admitted)) return 1;
    /* Capacity 1 is already full: the admitted writer MUST be parked. */
    q.present = 1;
    if (poc_thread_create(&t, writer, NULL)) return 2;
    if (poc_mutex_lock(&q.mutex)) return 3;
    while (!q.waiting_writer)
        if (poc_condition_wait(&q.admitted, &q.mutex)) return 4;
    poc_mutex_unlock(&q.mutex);
    if (close_owner() || close_owner() || poc_thread_join(&t, &result)) return 5;
    if (result != 10 || !q.present || q.accepted_writes) return 6;
    /* Reopen only after joining; now make an admitted reader park on empty. */
    if (poc_mutex_lock(&q.mutex)) return 7;
    q.present = 0;
    q.closed = 0;
    poc_mutex_unlock(&q.mutex);
    if (poc_thread_create(&t, reader, NULL)) return 8;
    if (poc_mutex_lock(&q.mutex)) return 9;
    while (!q.waiting_reader)
        if (poc_condition_wait(&q.admitted, &q.mutex)) return 10;
    poc_mutex_unlock(&q.mutex);
    if (close_owner() || poc_thread_join(&t, &result)) return 11;
    if (result != 20 || q.present || q.consumed_reads) return 12;
    if (poc_condition_destroy(&q.admitted) || poc_condition_destroy(&q.nonempty) ||
        poc_condition_destroy(&q.nonfull) || poc_mutex_destroy(&q.mutex)) return 13;
    puts("threads: parked writer/reader released by overlapping close, no phantom admission OK");
    return 0;
}
