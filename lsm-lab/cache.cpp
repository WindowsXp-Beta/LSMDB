#include "cache.h"

bool cache::ifExist(uint64_t key) {
    uint32_t hash[4];
    MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
    for (int i = 0; i < 4; i++) {
        if(!bloomFilter[hash[i]]) return false;
    }
    return true;
}

