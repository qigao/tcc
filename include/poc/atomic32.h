#ifndef TCC_POC_ATOMIC32_H
#define TCC_POC_ATOMIC32_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Dedicated 4-byte storage; NEVER reinterpret a C11 _Atomic or Salts atomic. */
typedef struct poc_atomic32 {
    volatile uint32_t value;
} poc_atomic32;
#define POC_ATOMIC32_INIT(value_) { (value_) }
/* All operations are sequentially consistent; values must be naturally aligned. */
uint32_t poc_atomic32_load(const poc_atomic32 *p);
void poc_atomic32_store(poc_atomic32 *p, uint32_t v);
uint32_t poc_atomic32_exchange(poc_atomic32 *p, uint32_t v);
uint32_t poc_atomic32_fetch_add(poc_atomic32 *p, uint32_t delta);
/* On failure update *expected with observed value; on success leave it equal. */
int poc_atomic32_compare_exchange(poc_atomic32 *p, uint32_t *expected, uint32_t desired);
#ifdef __cplusplus
}
#endif
#endif
