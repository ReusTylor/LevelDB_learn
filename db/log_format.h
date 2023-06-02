// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Log format information shared by reader and writer.
// See ../doc/log_format.md for more detail.
//  levelDB日志格式。包含了levelDB日志读取和写入之间共享的日志格式信息
#ifndef STORAGE_LEVELDB_DB_LOG_FORMAT_H_
#define STORAGE_LEVELDB_DB_LOG_FORMAT_H_

namespace leveldb {
namespace log {

// log通过事务写，事务解析，如果写了一半出现了崩溃，解析时候就无法解析出来半条record
enum RecordType {
  // 定义了日志记录的类型
  // 主要用于标识日志记录的不同部分或者日志记录的状态，以便在读取和写入日志时能够正确地处理和解析日志数据。
  // Zero is reserved for preallocated files
  kZeroType = 0, // 保留值，预分配文件时使用

  kFullType = 1, // 完整的日志记录类型，一条完整的日志记录

  // For fragments
  kFirstType = 2,
  kMiddleType = 3,
  kLastType = 4
};
static const int kMaxRecordType = kLastType; // 日志记录类型的最大值

static const int kBlockSize = 32768; // 日志块的大小

// Header is checksum (4 bytes), length (2 bytes), type (1 byte).
// 日志头部的大小：checksum（4字节） 长度（2字节） 类型（1字节）
static const int kHeaderSize = 4 + 2 + 1;

}  // namespace log
}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_LOG_FORMAT_H_
