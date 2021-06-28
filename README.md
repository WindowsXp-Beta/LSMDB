# LSMDB
A database implementation based on simplified LSM tree(Log-structured Merge Tree)
> Project of SE-2322 Advanced data structure
> @SJTU-SE

## Compile

I suggest using CLion to compile.(That's what I do​ :smile:

I've provided two CMakeLists files. One in main directory and one in memTable directory.

> So You can also use CMake to compile it by yourself.

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

## performance

Check [LSM_Tree lab report](./LSM Tree实验报告/LSM_Tree实验报告.pdf) to see the performance tests and results.

## storage structure

### In memory

1. memtable.
2. SSTable except the value part.

### In disk

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

