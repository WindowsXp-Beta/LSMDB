//
// Created by 魏新鹏 on 2021/5/31.
//

#ifndef LSMTREE_DEBUG_SSTABLE_H
#define LSMTREE_DEBUG_SSTABLE_H

#include "cache.h"
#include <string>

class SSTable{
private:
    cache* SSTcache;
    char* SSTvalue;

public:
    explicit SSTable(cache *cacheTmp = nullptr, char* valueTmp = nullptr):SSTcache(cacheTmp),SSTvalue(valueTmp){}
    ~SSTable() {delete SSTvalue;}
    cache* getCache() {return SSTcache;}
    uint64_t getSize() {return SSTcache -> getSize();}
    uint64_t getMin() {return SSTcache -> getMin();}
    uint64_t getMax() {return SSTcache -> getMax();}
    uint64_t getKey(int index) {return (SSTcache -> getKey())[index];}
    uint64_t getTime() {return (SSTcache -> getHead()).timeFlag;}
    std::string getValue(uint64_t index);
    char* getValueArray() {return SSTvalue;}
};
#endif //LSMTREE_DEBUG_SSTABLE_H