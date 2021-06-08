#pragma once

#include "kvstore_api.h"
#include "memTable/MemTable.h"
#include "cache.h"
#include "SSTable.h"
#include <list>
#include <deque>
#include <iostream>
#include <fstream>
#include "utils.h"


class KVStore : public KVStoreAPI {
	// You can add your implementation here
private:
    MemTable memTable;
	uint64_t timeFlag;
	std::deque< std::list< cache* >* > cacheList;
	std::string stoDir;
	std::deque< std::vector<int> > slot;//slot用来记录每层的空位

	/* private function */
	std::string getFilePath(uint32_t level, int fileName);
	std::string getFolderPath(uint32_t level);
	SSTable* readSST(uint32_t level, cache*);
	void writeSST(uint32_t level, int fileName, SSTable*);
	void deleteSST(uint32_t level, int fileName);
	cache* readCache(std::string&);
	SSTable* createSSTable();
	void compactor(SSTable* newSSTable);
public:
	explicit KVStore(const std::string &dir);

	~KVStore();

	uint32_t getSize() {
	    return memTable.getSize();
	}

	void put(uint64_t key, const std::string &s) override;

	std::string get(uint64_t key) override;

	bool del(uint64_t key) override;

	void reset() override;
};