// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "util/hash.h"

#include <cstring>

#include "util/coding.h"

// The FALLTHROUGH_INTENDED macro can be used to annotate implicit fall-through
// between switch labels. The real definition should be provided externally.
// This one is a fallback version for unsupported compilers.
#ifndef FALLTHROUGH_INTENDED
#define FALLTHROUGH_INTENDED \
  do {                       \
  } while (0)
#endif

namespace leveldb {

// 采取逐个输入数据的方式计算哈希值，
uint32_t Hash(const char* data, size_t n, uint32_t seed) {
  // Similar to murmur hash
  // m和r用于计算哈希值
  const uint32_t m = 0xc6a4a793;
  const uint32_t r = 24;

  // 
  const char* limit = data + n;
  // 根据种子值和数据长度计算初始哈希值 
  uint32_t h = seed ^ (n * m);

  // Pick up four bytes at a time
  while (data + 4 <= limit) {
    // DecodeFixed32 函数将4个字节的数据转换为一个32位整数 w
    uint32_t w = DecodeFixed32(data);
    data += 4;
    h += w;
    h *= m;
    h ^= (h >> 16);
  }

  // Pick up remaining bytes
  // 处理剩余的字节
  switch (limit - data) {
    // murmur hash中处理剩余字节的常见方式
    case 3:
      h += static_cast<uint8_t>(data[2]) << 16;
      FALLTHROUGH_INTENDED;
    case 2:
      h += static_cast<uint8_t>(data[1]) << 8;
      FALLTHROUGH_INTENDED;
    case 1:
      h += static_cast<uint8_t>(data[0]);
      h *= m;
      h ^= (h >> r);
      break;
  }
  return h;
}

}  // namespace leveldb

// MurmurHash 算法的核心思想是通过对输入数据进行一系列的位操作和混合操作，以产生一个哈希值。
// 它采用分布均匀的方式将输入数据映射到一个较大的输出空间，通常是一个32位或64位的哈希值。
