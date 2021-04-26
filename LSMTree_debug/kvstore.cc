#include "kvstore.h"
#include <string>
#include "MurmurHash3.h"
#include "utils.h"

KVStore::KVStore(const std::string &dir): KVStoreAPI(dir)
{
    stoDir = dir;
    timeFlag = 1;
    std::string level0 = stoDir + "/level-0";
    utils::mkdir(level0.data());
    std::list< cache* > *listZero = new std::list< cache* >;//level0's cache
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
    if (!memTable.isFull(key ,s.length())) {
        memTable.addEntry(key, s);
    }
    else {
        /* identify where to insert cache */
        uint64_t max = memTable.getMax();
        std::list< cache* >::iterator p = cacheList[0] -> begin();
        int i = 0;
        while (p != cacheList[0] -> end()) {
            if ((*p) -> getMax() > max) break;
            p++;
            i++;
        }
        cache * newCache = new cache();
        cacheList[0] -> insert(p, newCache);

        std::stringstream fmt;
        fmt << stoDir << "/level-0/" << i << ".sst";
        std::string writeLevel0 = fmt.str();
        std::ofstream outFile(writeLevel0, std::ios::out|std::ios::binary); //ios::out会清除文件中原来的内容


        /* Header */
        uint64_t size = memTable.getSize();
        Header headPart(timeFlag, size, max, memTable.getMin());
        newCache -> setHead(headPart);

        /* Bloom Filter part */
        bool * bloomFilter = memTable.getBloom();
        newCache -> setBloom(bloomFilter);


        /* index part */
        uint64_t * newKeyArray = new uint64_t [size + 1];
        uint32_t * newOffsetArray = new uint32_t [size + 1];
        uint32_t offset = 32 + 10240 + 12 * (size + 1);//initial offset
        Entry ** memTableContent = memTable.getWhole();

        for (i = 0; i < size; i++) {
            newKeyArray[i] = memTableContent[i] -> key;
            newOffsetArray[i] = offset;
            offset += (memTableContent[i] -> value.length());
        }
        newOffsetArray[i] = offset;
        newCache -> setKey(newKeyArray);
        newCache -> setOffset(newOffsetArray);

        /* write cache */
        outFile.write((char*)&headPart, sizeof(headPart));
        outFile.write((char*)bloomFilter, 10240);
        outFile.write((char*)newKeyArray, (size + 1) * sizeof(uint64_t));
        outFile.write((char*)newOffsetArray,(size + 1) * sizeof(uint32_t));

        /* value part */
        const char * value;
        for (int i = 0; i < size; i++) {
            value = memTableContent[i] -> value.data();
            outFile.write(value, strlen(value));
        }
        outFile.close();
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
        if (*ret_str_p == "~DELETE~") ret_str_p -> clear();
        return *ret_str_p;
    }
    else { //search in cache if find goto disk to get it
        uint64_t bigTime = 0, index = 0;
        uint32_t offset = 0, length = 0;
        uint32_t level = 0;
        int j = 0;//j records the index of cache in list
        for (int i = 0; i < cacheList.size(); i++) {
            j = 0;
            std::list< cache* >::iterator p = cacheList[i] -> begin();
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
