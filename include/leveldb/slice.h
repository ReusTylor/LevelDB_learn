// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Slice is a simple structure containing a pointer into some external
// storage and a size.  The user of a Slice must ensure that the slice
// is not used after the corresponding external storage has been
// deallocated.
//
// Multiple threads can invoke const methods on a Slice without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Slice must use
// external synchronization.

#ifndef STORAGE_LEVELDB_INCLUDE_SLICE_H_
#define STORAGE_LEVELDB_INCLUDE_SLICE_H_

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

#include "leveldb/export.h"

namespace leveldb {

class LEVELDB_EXPORT Slice {
 public:
  // Create an empty slice.
  // 默认构造函数，创建一个空的slice
  Slice() : data_(""), size_(0) {} 

  // Create a slice that refers to d[0,n-1].
  // 创建d指向的数据的slice对象，大小为n
  Slice(const char* d, size_t n) : data_(d), size_(n) {}

  // Create a slice that refers to the contents of "s"
  // 创建一个引用指定string对象s中的数据slice对象
  Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}

  // Create a slice that refers to s[0,strlen(s)-1]
  // 大小为字符串的长度
  Slice(const char* s) : data_(s), size_(strlen(s)) {}

  // Intentionally copyable.
  // 复制构造函数和赋值运算符重载函数，使用默认实现
  Slice(const Slice&) = default;
  Slice& operator=(const Slice&) = default;

  // Return a pointer to the beginning of the referenced data
  // 返回被引用数据的起始指针
  const char* data() const { return data_; }

  // Return the length (in bytes) of the referenced data
  // 返回被引用数据的大小
  size_t size() const { return size_; }

  // Return true iff the length of the referenced data is zero
  // 判断被引用数据大小是否为0
  bool empty() const { return size_ == 0; }

  // Return the ith byte in the referenced data.
  // REQUIRES: n < size()
  // 重载 [] 运算符，返回被引用数据中索引为 n 的字节。前提是 n 必须小于数据的大小。
  char operator[](size_t n) const {
    assert(n < size());
    return data_[n];
  }

  // Change this slice to refer to an empty array
  // 将slice对象置为空
  void clear() {
    data_ = "";
    size_ = 0;
  }

  // Drop the first "n" bytes from this slice.
  // 移除数据的前n位
  void remove_prefix(size_t n) {
    assert(n <= size());
    data_ += n;
    size_ -= n;
  }

  // Return a string that contains the copy of the referenced data.
  // 返回一个被引用数据拷贝的string对象
  std::string ToString() const { return std::string(data_, size_); }

  // Three-way comparison.  Returns value:
  // 比较当前slice对象和另一个slice对象b的大小关系，返回值
  //   <  0 iff "*this" <  "b",
  //   == 0 iff "*this" == "b",
  //   >  0 iff "*this" >  "b"
  int compare(const Slice& b) const;

  // Return true iff "x" is a prefix of "*this"
  // 判断x是否是该slice的前缀
  bool starts_with(const Slice& x) const {
    return ((size_ >= x.size_) && (memcmp(data_, x.data_, x.size_) == 0));
  }

 private:
  //指向外部存储的指针 data_存储数据地址
  const char* data_; 
  //size_ 表示数据的大小 "size_t"
  size_t size_;
};

// 判断两个slice对象大小是否相等
// 在类外实现的函数，想要内联的话就加上inline
// 类内的成员函数默认都是内联的
// 没有直接使用compare函数，因为判断相等并不需要进行完整字符串的比较
inline bool operator==(const Slice& x, const Slice& y) {
  return ((x.size() == y.size()) &&
          (memcmp(x.data(), y.data(), x.size()) == 0));
}

// !=
inline bool operator!=(const Slice& x, const Slice& y) { return !(x == y); }

inline int Slice::compare(const Slice& b) const {
  const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
  int r = memcmp(data_, b.data_, min_len);
  if (r == 0) {
    if (size_ < b.size_)
      r = -1;
    else if (size_ > b.size_)
      r = +1;
  }
  return r;
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_SLICE_H_
