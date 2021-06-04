//
// Created by 魏新鹏 on 2021/4/24.
//

#include "memTable/MemTable.h"
#include "kvstore.h"
#include "utils.h"
#include <iostream>
#include <fstream>

int main() {
        KVStore m("../data");
        std::string s;
        int i = 0;
//        m.put(1, "abc");
//        for (i = 1; i < 4; i++) {
//            std::cin >> s;
//            m.put(i, s);
//        }
//        std::cout << m.get(1) << '\t' << m.get(2) << '\t' <<m.get(3) << std::endl;
//
//        std::cout << m.del(1) << '\t' << m.get(1).empty() << std::endl;
        SSTable* newSST = m.readSST(0,0);
        std::cout << "size is" <<newSST -> getCache() ->getHead().size << "min is " << newSST -> getCache() ->getHead().min << "max is " << newSST -> getCache() ->getHead().max << "timeflag is " << newSST -> getCache() ->getHead().timeFlag << std::endl;

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

//    std::string s = "../data", s1 = "../data/level-0";
//    std::vector<std::string> ret;
//    std::cout << utils::dirExists(s) << std::endl;
//    if (utils::mkdir(s1.data())) std::cout << "Create failed\n";
//    std::cout << utils::dirExists(s1) << std::endl;
//    std::ofstream outFile("../data/level-0/test.txt", std::ios::out);
//    outFile<<s;
//    outFile.close();
//    outFile.open("../data/level-0/test1.txt", std::ios::out);
//    outFile<<s1;
//    outFile.close();
//    std::cout << utils::scanDir(s1, ret) << std::endl;
//    for(int i = 0; i < ret.size(); i++) {
//        std::cout<<ret[i]<<'\t';
//    }
}