#include "kvstore.h"
#include <string>
#include <cstring>
#include "MurmurHash3.h"
#include "utils.h"


SSTable *KVStore::readSST(uint32_t level, uint32_t index) {
    std::stringstream fmt;
    fmt << stoDir << "/level-" << level << "/" << index << ".sst";
    std::string resultDir = fmt.str();
    std::ifstream inFile(resultDir, std::ios::in|std::ios::binary);
    auto iterator = cacheList[level] -> begin();
    while(index--) {
        iterator++;
    }
    cache* sstCache = *iterator;
    uint64_t size = sstCache -> getSize();
    uint32_t valueSize = (sstCache -> getOffset())[size] - (sstCache -> getOffset())[0];
    char * newValueArray = new char [valueSize];
    inFile.read((char*)newValueArray, valueSize * sizeof(char));
    auto* newSStable = new SSTable(sstCache, newValueArray);
    return newSStable;
}

void KVStore::writeSST(uint32_t level, uint32_t index, SSTable* ssTable) {
    std::stringstream fmt;
    fmt << stoDir << "/level-" << level << "/" << index << ".sst";
    std::string writeLevel = fmt.str();
    std::ofstream outFile(writeLevel, std::ios::out|std::ios::binary); //ios::out会清除文件中原来的内容
    cache* sstCache = ssTable -> getCache();
    Header wHeader = sstCache -> getHead();
    uint64_t size = sstCache -> getSize();
    uint32_t valueSize = (sstCache -> getOffset())[size] - (sstCache -> getOffset())[0];
    outFile.write((char*)(&wHeader), sizeof(wHeader));
    outFile.write((char*)(sstCache -> getBloom()), 10240);
    outFile.write((char*)(sstCache -> getKey()), (size + 1) * sizeof(uint64_t));
    outFile.write((char*)(sstCache -> getOffset()), (size + 1) * sizeof(uint32_t));
    outFile.write((char*)(ssTable -> getValueArray()), valueSize);
}

KVStore::KVStore(const std::string &dir): KVStoreAPI(dir)
{
    stoDir = dir;
    timeFlag = 1;
    std::string level0 = stoDir + "/level-0";
    utils::mkdir(level0.data());
    auto *listZero = new std::list< cache* >;//level0's cache
    cacheList.push_back(listZero);
}

KVStore::~KVStore()
{
    while(!cacheList.empty()){
        delete cacheList.front();
        cacheList.pop_front();
    }
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
    if (!memTable.addEntry(key, s)) {
        int iterator;
        /* Header */
        uint64_t max = memTable.getMax();
        uint64_t min = memTable.getMin();
        uint64_t size = memTable.getSize();
        Header headPart(timeFlag, size, max, min);

        /* Bloom Filter part */
        bool * bloomFilter = memTable.getBloom();

        /* index part */
        uint64_t * newKeyArray = new uint64_t [size + 1];
        uint32_t * newOffsetArray = new uint32_t [size + 1];
        uint32_t offset = 32 + 10240 + 12 * (size + 1);//initial offset
        Entry* *memTableContent = memTable.getWhole();

        for (iterator = 0; iterator < size; iterator++) {
            newKeyArray[iterator] = memTableContent[iterator] -> key;
            newOffsetArray[iterator] = offset;
            offset += (memTableContent[iterator] -> value.length());
        }
        newOffsetArray[iterator] = offset;
        cache* newCache = new cache(headPart, bloomFilter, newKeyArray, newOffsetArray);

        /* value part */
        uint32_t valueSize = offset - (32 + 10240 + 12 * (size + 1));
        char* newValueArray = new char[valueSize];
        uint32_t cpyStart = 0;
        const char* value;
        for (iterator = 0; iterator < size; iterator++) {
            value = memTableContent[iterator] -> value.data();
            memcpy(newValueArray + cpyStart, value, strlen(value));
            cpyStart += strlen(value);
        }
        auto* newSSTable = new SSTable(newCache, newValueArray);
        
        delete []memTableContent;
        /* begin write sstable */
        if(cacheList[0] -> size() < 2) {//dont need compaction
            uint32_t index = 0;
            auto p = cacheList[0] -> begin();
            for(;p != cacheList[0] -> end();p++, index++) {
                if ((*p) -> getMax() > max) break;
            }
            writeSST(0, index, newSSTable);
            cacheList[0] -> insert(p, newCache);
            delete newSSTable;
        }
        else {//need compaction
            int nextLine = 1;
            uint64_t largestTime = timeFlag;
            
            /* 初始时，sstList只有第0行的两个sst和将要写入第0行而导致溢出的那个sst */
            std::vector<SSTable*> sstList;
            sstList.push_back(readSST(0,0));
            sstList.push_back(readSST(0,1));
            sstList.push_back(newSSTable);
            while(true) {
                uint64_t minimum = -1, maximum = 0;
                int minIndex = -1, maxIndex = -1;
                for(auto & p : sstList) {
                    minimum = p -> getMin() > minimum ? minimum : p -> getMin();
                    maximum = p -> getMax() > maximum ? p -> getMax() : maximum;
                }
                if(nextLine < cacheList.size()) {//nextLine exist
                    for(auto it = (cacheList[nextLine] -> begin()); it != (cacheList[nextLine] -> end()); it++) {
                        if(minimum > ((*it) -> getMax())) minIndex++;
                        if(maximum > ((*it) -> getMin())) maxIndex++;
                    }
                    for (iterator = minIndex + 1; iterator <= maxIndex; iterator++) {
                        sstList.push_back(readSST(nextLine, iterator));
                    }
                }

                /* begin compaction */
                /* 最后将所有sstable归并成一个大的entry数组 */
                std::vector<Entry> entryList;
                int *pointer = new int[sstList.size()]();//pointer数组保存每个sstable遍历到第几个元素
                while(true) {
                    uint64_t smallest = -1, maxTimeFlag = 0;
                    std::list<int> ownerList;
                    int owner = -1;
                    for(int i = 0; i < sstList.size(); i++) {
                        if(pointer[i] < 0) continue;//pointer[i]等于-1意味着其已经遍历完成
                        uint64_t smallTmp = sstList[i] -> getKey(pointer[i]);
                        if(smallest > smallTmp) {
                            ownerList.clear();
                            ownerList.push_back(i);
                            owner = i;
                            smallest = smallTmp;
                            maxTimeFlag = sstList[i] -> getTime();
                        }
                        else if(smallest == smallTmp) {
                            if(sstList[i] -> getTime() > maxTimeFlag) {
                                owner = i;
                                maxTimeFlag = sstList[i] -> getTime();
                            }
                            ownerList.push_back(i);
                        }
                    }
                    if(owner < 0) break;
                    entryList.emplace_back(smallest, sstList[owner] -> getValue(pointer[owner]));
                    for(auto & p:ownerList) {
                        pointer[p]++;
                        if(pointer[p] == (sstList[p] -> getSize())) {//if reach size
                            pointer[p] = -1;
                        }
                    }
                }
                /* 释放SSTable */
                while(!sstList.empty()) {
                    delete sstList.back();
                    sstList.pop_back();
                }

                /* rebuild SSTable */
                for(int p = 0; p < entryList.size();) {
                    newCache = new cache();
                    size = 0;
                    min = p;//p为entryList中的下标
                    while(newCache -> addEntry(entryList[p].key, entryList[p].value) && p < entryList.size()) {
                        p++;
                        size++;
                    }
                    max = p - 1;
                    newCache -> setHead(Header(largestTime, size, entryList[max].key, entryList[min].key));
                    
                    newKeyArray = new uint64_t[size + 1];
                    newOffsetArray = new uint32_t[size + 1];
                    offset = 32 + 10240 + 12 * (size + 1);//initial offset
                    for(iterator = 0; iterator < size; iterator++){
                        newKeyArray[iterator] = entryList[min + iterator].key;
                        newOffsetArray[iterator] = offset;
                        offset += entryList[iterator + min].value.length();
                    }
                    newOffsetArray[iterator] = offset;
                    newCache -> setKey(newKeyArray);
                    newCache -> setOffset(newOffsetArray);

                    valueSize = offset - (32 + 10240 + 12 * (size + 1));
                    newValueArray = new char[valueSize];
                    cpyStart = 0;
                    for(iterator = min; iterator <= max; iterator++) {
                       value = entryList[iterator].value.data();
                       memcpy(newValueArray + cpyStart, value, strlen(value));
                       cpyStart += strlen(value);
                    }
                    newSSTable = new SSTable(newCache, newValueArray);
                    sstList.push_back(newSSTable);
                }
                delete []pointer;

                /* write SSTable and update cache */
                int overlap = maxIndex - minIndex;

                if(nextLine >= cacheList.size()) {//如果下一层空，直接顺序写入
                    auto* newCacheList = new std::list<cache*>;
                    cacheList.push_back(newCacheList);
                    for(iterator = 0; iterator < sstList.size(); iterator++) {
                        newCacheList -> push_back(sstList[iterator] -> getCache());
                        writeSST(nextLine, iterator, sstList[iterator]);
                    }
                    while(!sstList.empty()) {
                        delete sstList.back() -> getValueArray();//删除char*
                        sstList.pop_back();
                    }
                    break;//finish compaction
                }
                /* 下一层能容纳 */
                else if( (cacheList[nextLine] -> size() + sstList.size() - overlap) <= (2 << nextLine) ) {
                    /* 先改文件名，从后往前改，一直改到maxIndex + 1 */
                    int incr = sstList.size() - overlap;
                    for(int i = (cacheList[nextLine] -> size() - 1); i > maxIndex; i--) {
                        std::stringstream fmt;
                        fmt << stoDir << "/level-" << nextLine << "/" << i + incr << ".sst";
                        std::string newName = fmt.str();
                        fmt.clear();
                        fmt << stoDir << "/level-" << nextLine << "/" << i << ".sst";
                        std::string oldName = fmt.str();
                        rename(oldName.data(), newName.data());
                    }
                    /* 替换重叠部分并更新cache */
                    int start = minIndex + 1;
                    auto p = cacheList[nextLine] -> begin();
                    while( start-- ) p++;
                    for(int i = minIndex + 1; i <= maxIndex; i++) {
                        cacheList[nextLine] -> erase(p);//其中包含的cache已经在释放SSTable时释放
                        p++;
                    }
                    for(int i = 0; i < sstList.size(); i++) {
                        cacheList[nextLine] -> insert(p, sstList[i] -> getCache());
                        writeSST(nextLine, start + i, sstList[i]);
                    }
                    /* 释放sstList中的char* */
                    while(!sstList.empty()) {
                        delete sstList.back() -> getValueArray();//删除char*
                        sstList.pop_back();
                    }
                    break;
                }
                /* 下一层容纳不下，即要多次归并 */
                else {
                    //读入溢出的部分

                }

            }
        }
        timeFlag++;
        memTable.reset();
        memTable.addEntry(key, s);
    }
}

/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key)
{
    std::string *ret_str_p;
    if ((ret_str_p = memTable.search(key)) != nullptr) {//find in memTable
        if (*ret_str_p == "~DELETE~") return "";
        else return *ret_str_p;
    }
    else { //search in cache if find goto disk to get it
        uint64_t bigTime = 0, index = 0;
        uint32_t offset = 0, length = 0;
        uint32_t level = 0;
        int j = 0;//j records the index of cache in list
        for (int i = 0; i < cacheList.size(); i++) {
            j = 0;
            auto p = cacheList[i] -> begin();
            while (p != cacheList[i] -> end()) {//遍历一层
                if ((*p) -> ifExist(key)) {//在cache中存在
                    uint64_t offsetTmp = (*p) -> binSearch(key, length);//二分搜索
                    if ( offsetTmp != 0) {//二分找到
                        uint64_t timeTmp = (*p) -> getTime();
                        if (timeTmp > bigTime) {//如果时间戳更新
                            offset = offsetTmp;
                            bigTime = timeTmp;
                            level = i;
                            index = j;
                        }
                    }
                }
                j++;
                p++;
            }
        }

        std::stringstream fmt;
        fmt << stoDir << "/level-" << level << "/" << index << ".sst";
        std::string resultDir = fmt.str();
        std::ifstream inFile(resultDir, std::ios::in|std::ios::binary);
        inFile.seekg(offset, std::ios::beg);
        char *ret_str_char = new char[length + 1];
        memset(ret_str_char, 0, length + 1);
        inFile.read(ret_str_char, length);
        std::string ret_str(ret_str_char);
        if (ret_str == "~DELETE~") ret_str.clear();
        delete []ret_str_char;
        return ret_str;
    }
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key)
{
    std::string result = get(key);
    if (!result.empty()) {
        put(key, "~DELETE~");
        return true;
    }
    else return false;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset()
{
//    memTable.clear();
}

