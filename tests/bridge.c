#include "poc/bridge.h"
uint32_t POC_CALL poc_bridge_abi_version(void) { return POC_BRIDGE_ABI_VERSION; }
uint32_t POC_CALL poc_bridge_checksum(const uint8_t *data, uint32_t n) {
    uint32_t i, h = UINT32_C(2166136261);
    if (!data && n) return 0;
    for (i = 0; i < n; ++i) { h ^= data[i]; h *= UINT32_C(16777619); }
    return h;
}
int32_t POC_CALL poc_bridge_sum(const int32_t *data, uint32_t n) {
    uint32_t i;
    int32_t sum = 0;
    if (!data || n > 16) return -1;
    for (i = 0; i < n; ++i) { if (data[i] < 0 || sum > 100000 - data[i]) return -2; sum += data[i]; }
    return sum;
}
