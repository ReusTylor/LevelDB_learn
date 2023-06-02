// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_LOG_WRITER_H_
#define STORAGE_LEVELDB_DB_LOG_WRITER_H_

#include <cstdint>

#include "db/log_format.h"
#include "leveldb/slice.h"
#include "leveldb/status.h"

namespace leveldb {
// 日志写入类，将日志数据追加写到文件中
class WritableFile;

namespace log {
// Writer 封装日志写入的功能
class Writer {
 public:
  // Create a writer that will append data to "*dest".
  // "*dest" must be initially empty.
  // "*dest" must remain live while this Writer is in use.
  // 构造函数，创建一个写入对象，将数据追加到dest指向的可写文件中。
  // dest必须初始为空，并在写入期间保持有效。
  explicit Writer(WritableFile* dest);

  // Create a writer that will append data to "*dest".
  // "*dest" must have initial length "dest_length".
  // "*dest" must remain live while this Writer is in use.
  Writer(WritableFile* dest, uint64_t dest_length);

  // 禁止拷贝构造函数和赋值操作符，确保写入器对象不能被复制
  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;

  ~Writer();
  // 添加一条日志记录到写入器中
  // slice：待添加的数据，返回一个status对象，表示操作的状态
  Status AddRecord(const Slice& slice);

 private:
 //将物理记录写入到可写文件中，并计算CRC校验值
  Status EmitPhysicalRecord(RecordType type, const char* ptr, size_t length);

  // 指向可写文件的指针
  WritableFile* dest_; 

  // 当前快的偏移量，用于确定下一个记录的位置。
  int block_offset_;  // Current offset in block

  // crc32c values for all supported record types.  These are
  // pre-computed to reduce the overhead of computing the crc of the
  // record type stored in the header.
  // 各个支持的记录类型的CRC32C校验值
  uint32_t type_crc_[kMaxRecordType + 1];
};

}  // namespace log
}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_LOG_WRITER_H_
