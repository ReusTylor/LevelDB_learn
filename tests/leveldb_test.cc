#include "leveldb/db.h"
#include <iostream>

int main()
{
    // 声明
 leveldb::DB* mydb;
 leveldb::Options myoptions;
 leveldb::Status mystatus;

    // 创建
 myoptions.create_if_missing = true;
    mystatus = leveldb::DB::Open(myoptions, "testdb", &mydb);

    // 写入数据
 std::string key = "gonev";
 std::string value = "a handsome man";
 if (mystatus.ok()) {
 mydb->Put(leveldb::WriteOptions(), key, value);
    }

    // 读取数据
 std::string key_ = "gonev";
 std::string val_ = "";
 mydb->Get(leveldb::ReadOptions(), key_, &val_);

 std::cout << key_ << ": " << val_ << std::endl;
}


