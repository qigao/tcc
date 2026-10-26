#ifndef TCC_POC_ATOMIC_SMALL_H
#define TCC_POC_ATOMIC_SMALL_H
#include <stdint.h>
/* Natural, independently allocated 1-byte and 2-byte storage.
 * No C11 _Atomic layout casting, no adjacent-word CAS and no volatile
 * fallback. This module does NOT implement the entire stdatomic.h API. */
#ifdef __cplusplus
extern "C" {
#endif
typedef struct poc_atomic8 { volatile uint8_t value; } poc_atomic8;
typedef struct poc_atomic16 { volatile uint16_t value; } poc_atomic16;
#define POC_ATOMIC8_INIT(v_) { (v_) }
#define POC_ATOMIC16_INIT(v_) { (v_) }
uint8_t poc_atomic8_load(const poc_atomic8 *p);
void poc_atomic8_store(poc_atomic8 *p, uint8_t v);
uint8_t poc_atomic8_exchange(poc_atomic8 *p, uint8_t v);
uint8_t poc_atomic8_fetch_add(poc_atomic8 *p, uint8_t v);
int poc_atomic8_compare_exchange(poc_atomic8 *p, uint8_t *expected, uint8_t desired);
uint16_t poc_atomic16_load(const poc_atomic16 *p);
void poc_atomic16_store(poc_atomic16 *p, uint16_t v);
uint16_t poc_atomic16_exchange(poc_atomic16 *p, uint16_t v);
uint16_t poc_atomic16_fetch_add(poc_atomic16 *p, uint16_t v);
int poc_atomic16_compare_exchange(poc_atomic16 *p, uint16_t *expected, uint16_t desired);
#ifdef __cplusplus
}
#endif
#endif
