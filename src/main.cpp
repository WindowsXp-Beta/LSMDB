//
// Created by 魏新鹏 on 2021/4/24.
//

#include "MemTable.h"
#include "kvstore.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
using namespace std;

int main() {
//    KVStore m("../../../../../../../../Volumes/其它/data");
    KVStore m("../tests/data");
    std::string s;
   int n;
   std::cout << "input test 次数" << std::endl;
   std::cin >> n;
   int i = 0;
   int key = 0;
   for (i = 0; i < n; i++) {
       std::cin >> key;
       std::cin >> s;
       m.put(key, s);
   }
    std::cout << m.get(4) << std::endl << m.get(10) << std::endl <<m.get(2) << std::endl;
//    m.reset();
//        std::ifstream inFile("../data/level-0/0.sst", std::ios::in|std::ios::binary);
//        char length[20];
//        Header h;
//        uint64_t key;
//        uint32_t offset;
//        inFile.seekg(10272,std::ios::beg);
//
//        inFile.read((char *)&key, sizeof(key));
//        std::cout << inFile.gcount() << std::endl << key << std::endl;
//        inFile.read((char *)&key, sizeof(key));
//        std::cout << inFile.gcount() << std::endl << key << std::endl;
//
//        inFile.read((char *)&offset, sizeof(offset));
//        std::cout << inFile.gcount() << std::endl << offset << std::endl;
//        inFile.seekg(10296,std::ios::beg);
//        inFile.read((char *)length, 20);
//        std::cout << inFile.gcount() << std::endl << length;
//    vector<string> folderList;
//    utils::scanDir("../data", folderList);
//    for(int i = 0; i < folderList.size(); i++) {
//        int levelName = atoi(folderList[i].data() + 6);
//        cout << levelName << endl;
//        cout << folderList[i] << endl;
//    }
//    cout << endl;
//    folderList.clear();
//    utils::scanDir("../../../../../../../../Volumes/其它/data", folderList);
//    for(int i = 0; i < folderList.size(); i++) {
//        int levelName = atoi(folderList[i].data() + 6);
//        cout << levelName << endl;
//        cout << folderList[i] << endl;
//    }
//    cout << endl;
}