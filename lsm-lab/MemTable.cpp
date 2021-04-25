//
// Created by 魏新鹏 on 2021/4/22.
//

#include "MemTable.h"
#include "MurmurHash3.h"


MemTable::MemTable() {
    bloomFilter = new bool[10240];
    capacity = 2086880;//initial capacity:2MB - 32 - 10240
}

MemTable::~MemTable() {
    delete bloomFilter;
}

bool MemTable::isFull(uint32_t length) {
    if (capacity - length <= 0) return true;
    else return false;
}

void MemTable::addEntry(uint64_t key, const std::string &value) {
    if (!skList.put(key, value)) {
        uint32_t hash[4];
        MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
        for (int i = 0; i < 4; i++) {
            bloomFilter[i] = 1;
        }
    }
    capacity -= 12 + value.length();
}

std::string * MemTable::search(uint64_t key) {
    return skList.get(key);
}

bool * MemTable::getBloom() {
    return bloomFilter;
}

uint64_t MemTable::getMax() {
    return SkList.empty() ? 0:SkList.back() -> last() -> entry.key;
}

uint64_t MemTable::getMin() {
    return SkList.empty() ? 0:SkList.back() -> front() -> entry.key;
}

uint32_t MemTable::getSize() {
    return skList.size();
}
