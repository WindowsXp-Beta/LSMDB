#pragma once

#include "kvstore_api.h"
#include "MemTable.h"
#include "cache.h"
#include <list>
#include <iostream>
#include <fstream>
#include "utils.h"


class KVStore : public KVStoreAPI {
	// You can add your implementation here
private:
    MemTable memTable;
	uint64_t timeFlag;
	std::list<cache*> cacheList;
	
public:
	KVStore(const std::string &dir);

	~KVStore();

	void put(uint64_t key, const std::string &s) override;

	std::string get(uint64_t key) override;

	bool del(uint64_t key) override;

	void reset() override;

};