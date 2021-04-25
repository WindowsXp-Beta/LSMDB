//
// Created by 魏新鹏 on 2021/4/22.
//

#include "MemTable.h"
#include "../MurmurHash3.h"

#define DebugCap 20

MemTable::MemTable() {
    bloomFilter = new bool[10240];
    memset(bloomFilter, 0, 10240);
    capacity = DebugCap;
    //capacity = 2086868;
    //initial capacity:2MB - 32 - 10240 - 12
    //the last 12 is the useless Index just for calculating the length of last string
}

MemTable::~MemTable() {
    delete bloomFilter;
}

bool MemTable::isFull(uint64_t key, uint32_t length) {
    std::string *s;
    if ((s = skList.get(key)) == nullptr) {
        if ((int)(capacity - length) <= 0) return true;
        else return false;
    }
    else {
        if((int)(capacity - length + s -> length()) <= 0) return true;
        else return false;
    }
}

void MemTable::addEntry(uint64_t key, const std::string &value) {
    if (skList.put(key, value)) {
        uint32_t hash[4];
        MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
        for (int i = 0; i < 4; i++) {
            bloomFilter[hash[i]%10240] = true;
        }
    }
    capacity -= 12 + value.length();
}

std::string * MemTable::search(uint64_t key) {
    std::string * result = skList.get(key);
    if (result) {
        if ((*result) == "~DELETE~") return nullptr;
        else return result;
    }
    return nullptr;
}

void MemTable::remove(uint64_t key) {
    if (skList.remove(key)) {
        skList.put(key, "~DELETE~");
    }
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
    bloomFilter = new bool[10240];
    memset(bloomFilter, 0, 10240);
    capacity = DebugCap;
    //capacity = 2086868;
}
