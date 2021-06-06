//
// Created by 魏新鹏 on 2021/4/22.
//

#include "MemTable.h"
#include "../MurmurHash3.h"
#include <cstring>

#define DEBUGx
#define DebugCap 34

MemTable::MemTable() {
    bloomFilter = new bool[10240]();
    //initial capacity:2MB - 32 - 10240 - 12
    //the last 12 is an useless Index just for calculating the length of last string
    capacity = 2086868;
#ifdef DEBUG
    capacity = DebugCap;
#endif
}

MemTable::~MemTable() {
    /* Empty */
}

// bool MemTable::isFull(uint64_t key, uint32_t length) {
//     std::string *s;
//     if ((s = skList.get(key)) == nullptr) {
//         if (capacity - (int)length - 12) <= 0) return true;
//         else return false;
//     }
//     else {
//         if((int)(capacity - length + s -> length()) <= 0) return true;
//         else return false;
//     }
// }

bool MemTable::addEntry(uint64_t key, const std::string &value) {
    std::string *s;
    if ((s = skList.get(key)) != nullptr) {
        if((capacity = capacity + (int)(s -> length()) - (int)value.length()) < 0) return false;
    }
    else {
        if((capacity = capacity - (int)value.length() - 12) < 0) return false;
        uint32_t hash[4];
        MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
        for (unsigned int i : hash) {
            bloomFilter[i%10240] = true;
        }
    }
    skList.put(key, value);
    return true;
}

std::string * MemTable::search(uint64_t key) {
    std::string * result = skList.get(key);
    return result;
}

Entry ** MemTable::getWhole() {
    return skList.getWhole();
}

bool * MemTable::getBloom() {
    return bloomFilter;
}

uint64_t MemTable::getMax() {
    return skList.getMax();
}

uint64_t MemTable::getMin() {
    return skList.getMin();
}

uint32_t MemTable::getSize() {
    return skList.size();
}

void MemTable::reset() {
    skList.clear();
    bloomFilter = new bool[10240]();
    capacity = 2086868;
#ifdef DEBUG
    capacity = DebugCap;
#endif
}

bool MemTable::empty() {
    return skList.empty();
}