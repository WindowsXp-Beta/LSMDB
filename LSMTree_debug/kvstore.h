#pragma once

#include "kvstore_api.h"
#include "memTable/MemTable.h"
#include "cache.h"
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
public:
	KVStore(const std::string &dir);

	~KVStore();

	void put(uint64_t key, const std::string &s) override;

	std::string get(uint64_t key) override;

	bool del(uint64_t key) override;

	void reset() override;

};