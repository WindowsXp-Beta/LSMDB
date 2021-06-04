#include "cache.h"
#include "MurmurHash3.h"

bool cache::ifExist(uint64_t key) {
    if (key < headerPart.min || key > headerPart.max) return false;

    uint32_t hash[4];
    MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
    for (unsigned int i : hash) {
        if(!bloomFilter[i%10240]) return false;
    }
    return true;
}

uint64_t cache::binSearch(uint64_t key, uint32_t &length) {
    uint64_t size = headerPart.size;
    uint64_t right = 0, left = size - 1;
    while (right <= left) {
        uint64_t mid = (right + left)/2;
        if (keyArray[mid] == key) {
            length = offsetArray[mid + 1] - offsetArray[mid];
            return offsetArray[mid];
        }
        else if (keyArray[mid] < key) {
            right = mid + 1;
        }
        else {
            left = mid - 1;
        }
    }
    length = 0;
    return 0;
}

bool cache::addEntry(uint64_t key, const std::string & value) {
    if((capacity = capacity - (int)value.size() - 12) <= 0) return false;
    uint32_t hash[4];
    MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
    for (unsigned int i : hash) {
        bloomFilter[i%10240] = true;
    }
    return true;
}