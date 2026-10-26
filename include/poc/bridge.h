#ifndef TCC_POC_BRIDGE_H
#define TCC_POC_BRIDGE_H
#include <stdint.h>
#define POC_BRIDGE_ABI_VERSION 1u
#ifdef _WIN32
#define POC_EXPORT __declspec(dllexport)
#define POC_CALL __cdecl
#else
#define POC_EXPORT
#define POC_CALL
#endif
#ifdef __cplusplus
extern "C" {
#endif
POC_EXPORT uint32_t POC_CALL poc_bridge_abi_version(void);
POC_EXPORT uint32_t POC_CALL poc_bridge_checksum(const uint8_t *data, uint32_t n);
POC_EXPORT int32_t POC_CALL poc_bridge_sum(const int32_t *data, uint32_t n);
#ifdef __cplusplus
}
#endif
#endif
