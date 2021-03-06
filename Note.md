# LSM_TREE
A database implementation based on simplified LSM tree(Log-structured Merge Tree)
> Project of SE-2322 Advanced data structure
> @SJTU-SE

## Part1

### week8

**使用 跳表 实现 MemTable，完成基本的 PUT、GET、DEL、Reset 操作**

### week9

**实现 SSTable 的生成（包括索引、BloomFilter 的缓存等），此阶段没有 Compaction，因此有一个无限大的 Level 0**

## Part2

### week14-16

**阶段三：实现 Compaction 和其余逻辑。**

**阶段四（即最终提交）：完成测试实验和报告（可能需根据测试要求更改部分代码）。**

## 题目解读

**KEY, VALUE == unsigned long, std::string**

### 整体逻辑

#### 内存中存的东西

1. memtable 2. 现有SSTable的除数据外的部分。

#### 硬盘里存的东西

> Level n 层的文件数量上限为 $2^{n+1}$（即Level 0 是 2，Level 1 是 4，Level 2 是 8，……）
>
> 除了 Level 0 之外，每一层中各个文件的键值区间不相交。

```
./data
	|__level-0
		|__ l1_1.sst
		|__ l1_2.sst
	|__level-1
		|__ l2_1.sst
		...
	|_level-2
	...
```

#### 启动和关闭attention

1. 在启动时，需检查现有的数据目录中各层 SSTable 文件，并在内存中构建相应的缓存。

2. 系统在正常关闭时（可以实现在析构函数里面），应将 MemTable 中的所有数据以 SSTable 形式写回（类似于MemTable满了时的操作

### 内存

cache存储结构

```c++
std::list< std::list<cache*> *> cacheList;
```

![cj4DHO.md.jpg](https://z3.ax1x.com/2021/04/24/cj4DHO.md.jpg)

在PUT的时候只会写入`cacheList[0]`

在合并的时候才会写入`cacheList[1 ~ n]`，并有可能需要增加新的行。

#### SkipList

跳表是一个双链表（std::List），双链表的元素是指向*QuadList（QuadList是开在堆里的）的指针，QuadList是每个节点是QuadListNode（四联节点）的链表（因为是链表，所以只要保存头尾指针就行了），每个QuadListNode（四联节点）也是开在堆里的，而每个QuadListNode是直接存储的Entry的（which means 取Entry的地址（string或key）会取到QuadListNode在堆中对应部分的地址），因此析构or写入磁盘后清除都要把这些空间释放。

> delete 每一个std::List中的元素 => 调用QuadList的析构 => 调用QuadList的`clear()` => 删除每一个`QuadListNode`

### SSTable结构

![cLsY9g.md.png](https://z3.ax1x.com/2021/04/22/cLsY9g.md.png)

一个SSTable 2MB = 2048Kb = 2097152 bytes

Header: total: 32 bytes

​	时间戳 (unsigned long 64bits 8bytes) 

> 关于时间戳
>
> 在键值系统 初始化 or reset 之后，第一个生成的SSTable时间戳为1 （which means时间戳越大，数据越新）。注意，在启动时，因为要先读取disk中的SSTable，因此时间戳是从disk中最大的时间戳下一个开始的。

​	键值对数量 (unsigned long 64bits 8bytes) 

​	键最小值最大值(long 64bits 8bytes)

布隆过滤器 （用来判断键是否存在）10kb = 10240 Bytes

​	是一个bool（1 byte）数组，则数组长度为10240。

索引区: (key, offset) -> (unsigned long, unsigned int) 12 bytes

> 无符号32位整数范围：4294967296 与2MB相比，这个索引足够宽裕。。。

在SSTable中查找键值：因为索引是有序的，所以可以使用二分搜索在$\log n$的时间内完成查找。

### PUT

尝试在`memtable`中插入，如果键值已经存在，则进行替换，如果插入or替换后`memtable`的大小超过2MB，则不进行此次操作而是先将`memtable`转化成`SSTable`并保存在level0中，若level0已满，则进行合并。这些操作完成之后，再进行插入or替换。

### GET

先从`memtable`中搜，如果找到，结束查询。如果没有找到，则在**内存**里（因为SSTable的非数据部分是存在内存里的）**逐层**查看每一个SSTable，先用Bloom filter判断键是否在其中，如果在，二分查找。之后根据offset从硬盘中读取value。

> 如何确定这个这是哪个sstable，每次更新offset（如果二分找到了并且时间戳大就更新）时，<del>记录这个cache的层数，时间戳和最大值。</del>
>
> <del>为何使用层数和时间戳即可确定这个cache对应哪个文件？</del>
>
> <del>层数毋庸置疑需要，时间戳和最大值可以确定这是这个文件夹中的哪一个.sst，因为一个文件夹中可能有多个相同的时间戳，如果发生过一次</del>
>
> 见**一些实现的想法记录2**

从硬盘中读取逻辑：

### DEL

1. 然后执行`GET`，即在memtable和sstable的cache中搜索
2. 如果找到，返回值为真。
3. 直接调`PUT`插一条`(key,"~DELETE~")` 因为有可能DELETE替换后就超过2M了，这时就要写sstable，即使memtable中存在key也无所谓，反正PUT会进行替换。

### RESET

1. 删除所有层的SSTable文件和目录。
2. 清除内存。（两部分）

### 合并

见文档​ :dog:

几点注意

1. 在合并时，如果遇到相同键 K 的多条记录，通过比较时间戳来决定键 K 的最新值，时间戳大的记录被保留。
2. 在执行合并操作时，根据时间戳将相同键的多个记录进行合并，通常不需要对“删除标记”进行特殊处理。因为删除标记的时间戳往往最新。若有新的覆盖，时间戳也可以解决问题。
3. 合并完成后要更新内存中的缓存信息。
4. 若产生的文件数超出 Level 1 层限定的数目，则从 Level 1的 SSTable中，<del>优先选择时间戳最小的若干个文件（时间戳相等选择键最小的文件） ，使得文件数满足层数要求，</del> 这里要注意，是将时间戳大的值保留在这层，把时间戳小的值往下写。 以同样的方法继续向下一层合并（若没有下一层，则新建一层）。

### 一些实现的想法记录

> 主要是为了context switch回这个lab的时候能够快一点想起来上次结束的时候在干啥:sweat:

1. 如何建立SSTable的索引

   在要建表的时候，把memtable的skiplist整个读到一个新数组里，无法动态维护这部分，因为offset只有在size确定后才能确定。

   `offset[i] = 32 + 10240 + size * 12 + `$\Sigma S_i.length()$
   
2. 在cache中找到了key，怎么在`./data`找到对应的文件

   ![cj4DHO.md.jpg](https://z3.ax1x.com/2021/04/24/cj4DHO.md.jpg)

   考虑我的cache的结构，level0中`cache1.Max < cache2.Max`如果两个max相等，比较min，如果min也相等，emmm暂时还没想好（在大量数据存储时不常见），剩余层因区间不可相交，因此有`cachei.Min < cachei+1.Min && cachei.Max < cachei+1.Max`，因此在遍历一行的时候，只要记住这是第几个cache，然后sstable的命名采用`i.sst` `i`为其在一层中的序号。
   
3. 如何compaction

   不同的compaction总具有下述形式，上一层若干连续的块，下一层若干连续的块（范围overlap的部分）。合并成为连续的`(key,value)`数组，然后再顺序扫描这个key value数组，将其转化为若干cache，并将其重新写入硬盘。

   从disk中读出来，以`cache(header + bloom + keyArray + offSetArray) + value`的形式

   以`cache(header + bloom + keyArray + offSetArray) + value`为单位操作，命名为SStable

   ```c++
   struct SSTable {
     cache cachePart;
     char* valuePart;
   };
   SSTable* readSSTable() {
     read Header;
     new bloomFilter[];
     read bloomFilter;
     new KeyArray[];
     read KeyArray;
     new offsetArray[];
     read offsetArray;
     new char value[];
     read value;
     new SSTable(Header, bloomFilter, KeyArray, offsetArray);
     return SSTable;
   }
   void writeSSTable(SSTable sst){
     write Header;
     write bloomFilter;
     write KeyArray;
     write offsetArray;
     write value;
   }
   vector<struct SSTable> compaction(vector<struct SSTable>);//value就是一个char数组
   ```

   0层写1层：读0层原来的两个，读一层重叠的若干个，compaction，0层compaction完，如果超出一层，将键小的写到一层。

   1层写2层：读二层重叠的部分，compaction，如果超出三层，将键小的写到三层，读四层重叠的部分。

   当不超出时，完成归并。


合并逻辑：

> 在cache中新增一条记录name，用来记录对应的SSTable的文件名。

`std::vector<SSTable*> sstlist`存待写入下一行的所有SSTable。

它的初始值是上一行溢出的，即时间戳较小的，键较小的。

首先，遍历下一行（cache），在其中找（在cache中找）与其区间覆盖的，<del>读取cache的name存入一个vector中。</del>似乎用一个`std::vector<std::list<cache*>::iterator>`来存更合适。因为你之后还要删掉这些cache。

然后遍历这个vector，读取每一个SSTable，插入sstlist，至此sstlist构造完毕。

归并sstlist中的所有sstable，形成一个大的entry数组。并将sstlist删除。

遍历这个entry数组，构造cache和value part。此时的cache是不知道name的。然后将其插入sstlist中。

写入disk，分情况讨论：

- 下一层空，构造新的`List<cache*>`，插入cache（存在sstlist中每一个SSTable的cache），cache中的name从0开始增大。把list插到cachelist中。

- 下一层不空，但可以容纳所有SSTable，遍历存储重叠区域name的vector，sstlist中cache的name先取这些name，再从最大值`List<cache*>.size()`开始写。

- 下一层容纳不下所有要写入的SSTable。又分两种情况

  - 容量大于要写入的SSTable数量

    遍历`[0, minIndex]`到`[maxIndex + 1, size]`找到时间戳大的，键小的，把name加入存储可替换name的vector。读入，加入sstlist的最后。

    sstlist中cache的name先取vector中的name，再从`List<cache*>.size()`开始赋。

    此时sstlist即为往下一行归并的sstable。

  - 容量小于要写入的SSTable数量

    直接读入所有的[0, minIndex]与[maxIndex + 1, size]，将sstlist顺序填入，填满下一行，name从0开始。

