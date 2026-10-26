#include "poc/threads.h"
#include "poc/atomic32.h"
#include <stdio.h>
#include <string.h>

enum { PRODUCERS = 4, CONSUMERS = 4, ITEMS = 2500, CAPACITY = 7 };
typedef struct bounded_queue {
    poc_mutex mutex;
    poc_condition nonempty;
    poc_condition nonfull;
    int data[CAPACITY];
    unsigned head, tail, used;
    int closed, produced, consumed, errors, rejected;
    unsigned char seen[PRODUCERS * ITEMS];
} bounded_queue;
static bounded_queue q;
static poc_tss key;
static poc_atomic32 destructor_count = POC_ATOMIC32_INIT(0);
static int worker_ids[PRODUCERS + CONSUMERS];

static void key_destructor(void *value) {
    if (value) (void)poc_atomic32_fetch_add(&destructor_count, 1);
}
static int push(int value) {
    if (poc_mutex_lock(&q.mutex)) return -2;
    while (q.used == CAPACITY && !q.closed)
        if (poc_condition_wait(&q.nonfull, &q.mutex)) { poc_mutex_unlock(&q.mutex); return -2; }
    if (q.closed) { ++q.rejected; poc_mutex_unlock(&q.mutex); return -1; }
    q.data[q.tail] = value;
    q.tail = (q.tail + 1) % CAPACITY;
    ++q.used;
    ++q.produced;
    poc_condition_signal(&q.nonempty);
    poc_mutex_unlock(&q.mutex);
    return 0;
}
static int pop(int *value) {
    if (!value || poc_mutex_lock(&q.mutex)) return -2;
    while (!q.used && !q.closed)
        if (poc_condition_wait(&q.nonempty, &q.mutex)) { poc_mutex_unlock(&q.mutex); return -2; }
    if (!q.used) { poc_mutex_unlock(&q.mutex); return 1; }
    *value = q.data[q.head];
    q.head = (q.head + 1) % CAPACITY;
    --q.used;
    if (*value < 0 || *value >= PRODUCERS * ITEMS || q.seen[*value]++) ++q.errors;
    ++q.consumed;
    poc_condition_signal(&q.nonfull);
    poc_mutex_unlock(&q.mutex);
    return 0;
}
static int close_queue(void) {
    if (poc_mutex_lock(&q.mutex)) return -1;
    q.closed = 1;
    poc_condition_broadcast(&q.nonempty);
    poc_condition_broadcast(&q.nonfull);
    poc_mutex_unlock(&q.mutex);
    return 0;
}
static int produce(void *arg) {
    int i, id = *(int *)arg;
    if (poc_tss_set(&key, arg) || poc_tss_get(&key) != arg) return -1;
    /* Updating the same TLS key must retain just one destructor callback. */
    if (poc_tss_set(&key, arg) || poc_tss_get(&key) != arg) return -3;
    for (i = 0; i < ITEMS; ++i) if (push(id * ITEMS + i)) return -2;
    return 100 + id;
}
static int consume(void *arg) {
    int value, rc;
    if (poc_tss_set(&key, arg) || poc_tss_get(&key) != arg) return -1;
    /* Updating the same TLS key must retain just one destructor callback. */
    if (poc_tss_set(&key, arg) || poc_tss_get(&key) != arg) return -3;
    for (;;) {
        rc = pop(&value);
        if (rc == 1) break;
        if (rc != 0) return -2;
    }
    return 200 + *(int *)arg;
}
int main(void) {
    poc_thread threads[PRODUCERS + CONSUMERS] = {{0}};
    int i, result;
    memset(&q, 0, sizeof(q));
    if (poc_thread_create(NULL, produce, NULL) != -1) return 1;
    if (poc_thread_join(&threads[0], NULL) != -1) return 2;
    if (poc_condition_wait(NULL, NULL) != -1) return 3;
    if (poc_tss_set(&key, NULL) != -1) return 4;
    if (poc_mutex_init(&q.mutex) || poc_condition_init(&q.nonempty) ||
        poc_condition_init(&q.nonfull) || poc_tss_create(&key, key_destructor)) return 5;
    if (poc_mutex_init(&q.mutex) != -1) return 6;
    for (i = 0; i < PRODUCERS + CONSUMERS; ++i) worker_ids[i] = i;
    for (i = 0; i < CONSUMERS; ++i)
        if (poc_thread_create(&threads[PRODUCERS + i], consume, &worker_ids[PRODUCERS + i])) return 7;
    for (i = 0; i < PRODUCERS; ++i)
        if (poc_thread_create(&threads[i], produce, &worker_ids[i])) return 8;
    for (i = 0; i < PRODUCERS; ++i) {
        if (poc_thread_join(&threads[i], &result) || result != 100 + i) return 9;
        if (poc_thread_join(&threads[i], &result) != -1) return 10;
    }
    if (close_queue() || close_queue() || push(123) != -1) return 11;
    for (i = 0; i < CONSUMERS; ++i)
        if (poc_thread_join(&threads[PRODUCERS + i], &result) || result != 200 + PRODUCERS + i) return 12;
    if (q.errors || q.used || q.produced != PRODUCERS * ITEMS ||
        q.consumed != PRODUCERS * ITEMS || q.rejected != 1) return 13;
    for (i = 0; i < PRODUCERS * ITEMS; ++i) if (q.seen[i] != 1) return 14;
    if (poc_atomic32_load(&destructor_count) != PRODUCERS + CONSUMERS) return 15;
    if (poc_tss_destroy(&key) || poc_condition_destroy(&q.nonempty) ||
        poc_condition_destroy(&q.nonfull) || poc_mutex_destroy(&q.mutex)) return 16;
    if (poc_mutex_destroy(&q.mutex) != -1 || poc_tss_destroy(&key) != -1) return 17;
    puts("threads: bounded MPMC, close wakeup, join, TLS destructor OK");
    return 0;
}
