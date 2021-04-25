//
// Created by 魏新鹏 on 2021/4/23.
//

#ifndef LSM_LAB_CACHE_H
#define LSM_LAB_CACHE_H
#include "MurmurHash3.h"

class Index {
public:
    uint64_t key;
    uint32_t offset;
};

class Header {
public:
    uint64_t timeFlag;
    uint64_t size;
    uint64_t max;
    uint64_t min;
    Header(uint64_t timeTmp, uint64_t sizeTmp, uint64_t maxTmp, uint64_t minTmp):timeFlag(timeTmp), size(sizeTmp), max(maxTmp), min(minTmp){}
};

class cache {
public:
    cache(Header headTmp, bool * bloomTmp):headerPart(headTmp), bloomFilter(bloomTmp){}
    void setIndex(Index * indexTmp) {cacheIndex = indexTmp;}
    bool ifExist(uint64_t key); 
    
private:

    /* Header */
    Header headerPart;
    /* bloom */
    bool * bloomFilter;
    /* index */
    Index * cacheIndex;
};

#endif //LSM_LAB_CACHE_H