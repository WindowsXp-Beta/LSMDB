//
// Created by 魏新鹏 on 2021/4/16.
//

#ifndef LSM_LAB_ENTRY_H
#define LSM_LAB_ENTRY_H

#include <string>
struct Entry {
    uint64_t key; std::string value; //关键码、数值
    explicit Entry ( uint64_t k = 0, std::string v = std::string() ) : key ( k ), value ( v ) {}; //默认构造函数
    Entry ( Entry const &e ) : key ( e.key ), value ( e.value ) {}; //基于克隆的构造函数
    bool operator< ( Entry const &e ) const { return key <  e.key; }  //比较器：小于
    bool operator> ( Entry const &e ) const { return key >  e.key; }  //比较器：大于
    bool operator== ( Entry const &e ) const { return key == e.key; } //判等器：等于
    bool operator!= ( Entry const &e ) const { return key != e.key; } //判等器：不等于
}; //得益于比较器和判等器，从此往后，不必严格区分词条及其对应的关键码

#endif //LSM_LAB_ENTRY_H
