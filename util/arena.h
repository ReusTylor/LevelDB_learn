// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_UTIL_ARENA_H_
#define STORAGE_LEVELDB_UTIL_ARENA_H_

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace leveldb {

class Arena {
 // 内存分配器，在连续的内存块中分配内存
 public:
  Arena();

  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

  ~Arena();

  // Return a pointer to a newly allocated memory block of "bytes" bytes.
  // 用于分配指定字节数的内存块
  char* Allocate(size_t bytes);

  // Allocate memory with the normal alignment guarantees provided by malloc.
  // 分配指定字节数的内存块，并满足正常的内存对其要求
  char* AllocateAligned(size_t bytes);

  // Returns an estimate of the total memory usage of data allocated
  // by the arena.
  // 通过该Arena实例分配的内存总量的估计值
  size_t MemoryUsage() const {
    return memory_usage_.load(std::memory_order_relaxed);
  }

 private:
  // 用于分配指定字节数的内存块 
  char* AllocateFallback(size_t bytes);
  // 分配新的内存块，并将其添加到内部的块数组中
  char* AllocateNewBlock(size_t block_bytes);

  // Allocation state
  // 当前内存块的分配指针，指向下一个可用的内存位置
  char* alloc_ptr_;
  // 当前内存块中剩余的可用字节数
  size_t alloc_bytes_remaining_;

  // Array of new[] allocated memory blocks
  // Arena对象分配的内存块的数组
  std::vector<char*> blocks_;

  // Total memory usage of the arena.
  //
  // TODO(costan): This member is accessed via atomics, but the others are
  //               accessed without any locking. Is this OK?
  // Arena对象分配的内存总量的原子计数器
  std::atomic<size_t> memory_usage_;
};
// 分配指定字节数的内存块，如果当前剩余的字节数足够，则直接分配并返回指向该内存块的指针，否则调用AllocateFallback函数进行分配
inline char* Arena::Allocate(size_t bytes) { 
  // The semantics of what to return are a bit messy if we allow
  // 0-byte allocations, so we disallow them here (we don't need
  // them for our internal use).
  assert(bytes > 0);  //assert在程序中尽量不要使用，
  if (bytes <= alloc_bytes_remaining_) {  //判断需要的字节数是否<=当前内存剩余的字节数
    char* result = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;
    return result;
  }
  return AllocateFallback(bytes); //否则的话进入fallback
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_ARENA_H_
