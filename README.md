# LSMDB
A database implementation based on simplified LSM tree(Log-structured Merge Tree)
> Project of SE-2322 Advanced data structure

## Getting Started

I use CMake to compile all the tests and main entry(`src/main.cpp`).

Three tests(compaction, correctness, persistence) are in the `tests` directory.

The main entry is like a playground and you can create a LSMDB(aka. KVStore) object, specify the data path and play with it.

## Features

> support basic Database operations —— CRUD

1. PUT

   ```c++
   void KVStore::put(uint64_t key, const std::string value);
   ```

2. GET

   ```c++
   std::string KVStore::get(uint64_t key);
   ```

3. DELETE

   ```c++
   bool del(uint64_t key);
   ```

4. RESTART

   After restart, it will first scan the `./data` directory to build SSTable from existing files.

## Performance

Check [LSM_Tree实验报告](../docs/report/LSM_Tree实验报告.pdf) for a detailed and vivid report.

## Storage Structure

### memory

1. memtable.
2. SSTable except the value part.

### disk

> Level n 层的文件数量上限为 $2^{n+1}$（即Level 0 是 2，Level 1 是 4，Level 2 是 8，……）
>
> 除了 Level 0 之外，每一层中各个文件的键值区间不相交。

```
./data
	|__level-0
		|__ 0.sst
		|__ 1.sst
	|__level-1
		|__ 0.sst
		|__ 1.sst
		|__ 2.sst
		|__ 3.sst
	|_level-2
	...
```

