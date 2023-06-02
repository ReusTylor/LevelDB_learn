// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/log_writer.h"

#include <cstdint>

#include "leveldb/env.h"
#include "util/coding.h"
#include "util/crc32c.h"

namespace leveldb {
namespace log {

// 初始化记录类型的CRC32C 校验值数组
static void InitTypeCrc(uint32_t* type_crc) {
  for (int i = 0; i <= kMaxRecordType; i++) {
    char t = static_cast<char>(i);
    type_crc[i] = crc32c::Value(&t, 1);
  }
}
// 构造函数，创建一个写入对象，并将数据追加写到dest指向的文件中
// 同时初始化type_crc_数组
Writer::Writer(WritableFile* dest) : dest_(dest), block_offset_(0) {
  InitTypeCrc(type_crc_);
}

Writer::Writer(WritableFile* dest, uint64_t dest_length)
    : dest_(dest), block_offset_(dest_length % kBlockSize) {
  InitTypeCrc(type_crc_);
}

// 使用默认的析构函数
Writer::~Writer() = default;

// 将给定的数据片段作为一条日志记录添加到写入其中。
Status Writer::AddRecord(const Slice& slice) {
  const char* ptr = slice.data();//数据片段的指针指向ptr
  size_t left = slice.size(); // 将数据片段的长度赋值给left，表示需要处理的剩余数据长度

  // Fragment the record if necessary and emit it.  Note that if slice
  // is empty, we still want to iterate once to emit a single
  // zero-length record
  // 定义一个Status对象，表示操作状态。
  Status s;
  // 表示当前是否是记录的开始
  bool begin = true;
  // 通过循环来处理数据片段中的数据，直到所有数据都被处理完毕或出现错误。
  do {
    // 计算当前块中剩余的空间大小。block_offset_表示当前块的偏移量，即已经在当前块中写入的字节数。
    const int leftover = kBlockSize - block_offset_;
    assert(leftover >= 0);
    // 如果剩余空间不足以容纳头部大小，则切换到一个新的块。
    if (leftover < kHeaderSize) {
      // Switch to a new block
      // 如果剩余空间大于0，则填充尾部空间，确保不留下不完整的记录。
      // 在切换到一个新的块之前，将当前剩余块的空间用特定的填充字节填充起来。
      if (leftover > 0) {
        // Fill the trailer (literal below relies on kHeaderSize being 7)
        // static_assert(kHeaderSize == 7, ""); 是一个静态断言（static_assert），用于确保 kHeaderSize 的值为 7。
        // 这个断言是为了保证填充字节的长度与日志记录头部长度相同。
        static_assert(kHeaderSize == 7, "");
        dest_->Append(Slice("\x00\x00\x00\x00\x00\x00", leftover));
      }
      // 重置块的偏移量，表示新的块
      block_offset_ = 0;
    }

    // Invariant: we never leave < kHeaderSize bytes in a block.
    assert(kBlockSize - block_offset_ - kHeaderSize >= 0);

    // 计算当前块中可用的空间大小，减去头部大小。
    const size_t avail = kBlockSize - block_offset_ - kHeaderSize;
    // 计算当前片段的长度，取剩余数据长度和可用空间的较小值。
    const size_t fragment_length = (left < avail) ? left : avail;

    RecordType type;
    // 判断是否是记录的结尾
    const bool end = (left == fragment_length);
    if (begin && end) {
      type = kFullType;
    } else if (begin) {
      type = kFirstType;
    } else if (end) {
      type = kLastType;
    } else {
      type = kMiddleType;
    }

    // 调用这种方法将当前片段作为一条物理记录写入到文件中，将返回状态保存到s中
    s = EmitPhysicalRecord(type, ptr, fragment_length);
    // 指向下一个片段的起始位置
    ptr += fragment_length;
    // 从生于数据长度left中减去当前处理的片段长度fragment_length。为了跟踪剩余未处理的数据长度
    left -= fragment_length;
    // 将 begin 标志设置为 false，表示不再是记录的开始片段。在循环的后续迭代中，
    // 如果还有剩余数据需要处理，将会使用中间类型 kMiddleType 来表示。
    begin = false;
    // 判断是否继续迭代
    // 在每次迭代中，根据剩余空间和数据长度的关系，决定如何分片和处理数据。
    // 根据片段的位置和类型选择合适的记录类型 type，然后调用 EmitPhysicalRecord() 方法来处理当前片段，
    // 并将 s 更新为该方法的返回状态。
  } while (s.ok() && left > 0);
  return s;
}

// 将给定的数据片段写入日志文件
Status Writer::EmitPhysicalRecord(RecordType t, const char* ptr,
                                  size_t length) {
    
  // 断言检查确保长度等条件满足
  // 确保片段长度不超过两个字节
  assert(length <= 0xffff);  // Must fit in two bytes
  //确保当前的偏移量，记录头部长度和片段长度之和不超过块的大小。
  assert(block_offset_ + kHeaderSize + length <= kBlockSize);

  // Format the header
  // 创建长度为kHeaderSize的缓冲区buf，存储记录的头部数据
  char buf[kHeaderSize];
  buf[4] = static_cast<char>(length & 0xff);
  buf[5] = static_cast<char>(length >> 8);
  buf[6] = static_cast<char>(t);

  // Compute the crc of the record type and the payload.
  // 计算记录类型和片段数据的CRC校验值
  uint32_t crc = crc32c::Extend(type_crc_[t], ptr, length);
  crc = crc32c::Mask(crc);  // Adjust for storage
  EncodeFixed32(buf, crc);

  // Write the header and the payload
  // 将头部数据和片段数据写入日志文件。
  Status s = dest_->Append(Slice(buf, kHeaderSize));
  if (s.ok()) {
    s = dest_->Append(Slice(ptr, length));
    if (s.ok()) {
      s = dest_->Flush();
    }
  }
  block_offset_ += kHeaderSize + length;
  return s;
}

}  // namespace log
}  // namespace leveldb
