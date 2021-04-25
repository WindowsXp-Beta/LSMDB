#include "kvstore.h"
#include <string>
#include "MurmurHash3.h"

KVStore::KVStore(const std::string &dir): KVStoreAPI(dir)
{
    timeFlag = 0;
}

KVStore::~KVStore()
{
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
    if (memTable.isFull(s.length())) {
        std::ofstream outFile("data.dat", std::ios::out|std::ios::binary);
        uint64_t size = memTable.getSize();
        /* Header */
        Header headPart(timeFlag, size, memTable.getMax(), memTable.getMin());
        outFile.write((char*)&headPart, sizeof(headPart));
        /* Bloom Filter part */
        bool * bloomFilter = memTable.getBloom();
        outFile.write((char*)bloomFilter, 10240);
        /* new cache */
        cache * newCache = new cache(headPart, bloomFilter);
        /* index part */
        Index * newIndex = new Index[size];
        newCache -> setIndex(newIndex);
        uint32_t offset = 32 + 10240 + 12 * size;//initial offset
        Entry ** memTableContent = memTable.getWhole();
        for (int i = 0; i < size; i++) {
            newIndex[i].key = memTableContent[i] -> key;
            newIndex[i].offset = offset;
            offset += memTableContent[i] -> value.length() + 1;//plus one for the \0
        }
        outFile.write((char*)newIndex, size);
        cacheList.push_back(newCache);
        /* value part */
        const char * value;
        for (int i = 0; i < size; i++) {
            value = memTableContent[i] -> value.data();
            outFile.write(value, strlen(value) + 1);
        }

        timeFlag++;
    }
    else {
        memTable.addEntry(key, s);
    }
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key)
{
    std::string *ret_str;
    if ((ret_str = memTable.search(key)) != nullptr) {//find in memTable
        return *ret_str;
    }
    else {//search in cache if find goto disk to get it
        std::list<cache*>::iterator p = cacheList.begin();
        if ((*p) -> ifExist(key)) {

        }
    }
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key)
{
    if (memTable.remove(key)) return true;
	return false;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset()
{
    memTable.clear();
}
