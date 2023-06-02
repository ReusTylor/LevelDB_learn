// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_SKIPLIST_H_
#define STORAGE_LEVELDB_DB_SKIPLIST_H_

// Thread safety
// -------------
//
// Writes require external synchronization, most likely a mutex.
// Reads require a guarantee that the SkipList will not be destroyed
// while the read is in progress.  Apart from that, reads progress
// without any internal locking or synchronization.
//
// Invariants:
//
// (1) Allocated nodes are never deleted until the SkipList is
// destroyed.  This is trivially guaranteed by the code since we
// never delete any skip list nodes.
//
// (2) The contents of a Node except for the next/prev pointers are
// immutable after the Node has been linked into the SkipList.
// Only Insert() modifies the list, and it is careful to initialize
// a node and use release-stores to publish the nodes in one or
// more lists.
//
// ... prev vs. next pointer ordering ...

#include <atomic>
#include <cassert>
#include <cstdlib>

#include "util/arena.h"
#include "util/random.h"

namespace leveldb {

template <typename Key, class Comparator>
class SkipList {
 private:
  struct Node;// 私有嵌套结构体，表示跳表的节点

 public:
  // Create a new SkipList object that will use "cmp" for comparing keys,
  // and will allocate memory using "*arena".  Objects allocated in the arena
  // must remain allocated for the lifetime of the skiplist object.
  // 构造函数，用于创建一个新的跳表对象。接受一个比较器对象和一个存储区域作为参数，并初始化跳表的成员变量。
  // explicit 修饰类的构造函数，禁止编译器执行隐事类型转换，要求显式调用构造函数创建对象。
  explicit SkipList(Comparator cmp, Arena* arena); 

  SkipList(const SkipList&) = delete;
  SkipList& operator=(const SkipList&) = delete;

  // Insert key into the list.
  // REQUIRES: nothing that compares equal to key is currently in the list.
  // 插入操作函数,用于将给定的键插入到跳表中，要求跳表中不存在与给定键相等的条目
  void Insert(const Key& key);

  // Returns true iff an entry that compares equal to key is in the list.
  // 查询操作函数，用于判断跳表中是否在与给定键相等的条目
  bool Contains(const Key& key) const;

  // Iteration over the contents of a skip list
  // 迭代器类，用于在跳表中遍历内容
  class Iterator {
   public:
    // Initialize an iterator over the specified list.
    // The returned iterator is not valid.
    // 构造函数，用于初始化迭代器对象。接受一个指向SkipList对象的指针作为参数，并将其存储在list_成员变量中。
    explicit Iterator(const SkipList* list);

    // Returns true iff the iterator is positioned at a valid node.
    // 检查当前迭代器是否位于有效节点处。返回true或者false
    bool Valid() const;

    // Returns the key at the current position.
    // REQUIRES: Valid()
    // 返回当前节点的键值，返回一个对键值的常引用
    const Key& key() const;

    // Advances to the next position.
    // REQUIRES: Valid()
    // 将迭代器移动到下一个位置
    void Next();

    // Advances to the previous position.
    // REQUIRES: Valid()
    // 将迭代器移动到前一个位置
    void Prev();

    // Advance to the first entry with a key >= target
    // 迭代器移动到第一个键值大于等于目标键值的位置
    void Seek(const Key& target);

    // Position at the first entry in list.
    // Final state of iterator is Valid() iff list is not empty.
    // 将迭代器移动到跳跃表的第一个节点位置。
    void SeekToFirst();

    // Position at the last entry in list.
    // Final state of iterator is Valid() iff list is not empty.
    // 将迭代器移动到跳跃表的最后一个节点位置。
    void SeekToLast();

   private:
    //指向所属的Skiplist对象的指针
    const SkipList* list_;
    // 指向当前迭代器位置的节点的指针
    Node* node_;
    // Intentionally copyable
  };

 private:
  // 跳表的最大高度为12
  enum { kMaxHeight = 12 };
  // 返回跳表的最大高度
  inline int GetMaxHeight() const {
    return max_height_.load(std::memory_order_relaxed);
  }
  // 创建一个新的节点，并返回指向该节点的指针
  Node* NewNode(const Key& key, int height);
  // 随机生成一个节点的高度
  int RandomHeight();
  // 判断两个键值是否相等
  bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }

  // Return true if key is greater than the data stored in "n"
  // 判断给定的键值是否大于节点n中存储的键值。
  bool KeyIsAfterNode(const Key& key, Node* n) const;

  // Return the earliest node that comes at or after key.
  // Return nullptr if there is no such node.
  //
  // If prev is non-null, fills prev[level] with pointer to previous
  // node at "level" for every level in [0..max_height_-1].
  // 返回大于等于给定键值key的最早节点。如果不存在这样的节点，则返回nullptr。
  // 如果prev非空，会将每个层级[0..max_height_-1]的前一个节点的指针填充到prev数组中。
  Node* FindGreaterOrEqual(const Key& key, Node** prev) const;

  // Return the latest node with a key < key.
  // Return head_ if there is no such node.
  // 返回小于给定键值key的最新节点。
  Node* FindLessThan(const Key& key) const;

  // Return the last node in the list.
  // Return head_ if list is empty.
  // 返回跳跃表中的最后一个节点。
  Node* FindLast() const;

  // Immutable after construction
  // 比较键值的比较器
  Comparator const compare_;
  // 用于节点分配的内存池
  Arena* const arena_;  // Arena used for allocations of nodes
  // 指向跳表的头节点
  Node* const head_;

  // Modified only by Insert().  Read racily by readers, but stale
  // values are ok.
  // 原子整型的变量，表示整个跳表的高度
  std::atomic<int> max_height_;  // Height of the entire list

  // Read/written only by Insert().
  // 一个随机数生成器对象
  Random rnd_;
};

// Implementation details follow
template <typename Key, class Comparator>
struct SkipList<Key, Comparator>::Node { //跳表中节点的结构体定义。
  explicit Node(const Key& k) : key(k) {}

  Key const key; //用于存储节点的关键字

  // Accessors/mutators for links.  Wrapped in methods so we can
  // add the appropriate barriers as necessary.
  Node* Next(int n) { // 根据给定的层级n，返回指向下一个节点的指针。使用了memory_order_acquire内存序
    assert(n >= 0);
    // Use an 'acquire load' so that we observe a fully initialized
    // version of the returned Node.
    // 保证读取的值是其他线程的最新版本。
    return next_[n].load(std::memory_order_acquire);
  }
  void SetNext(int n, Node* x) {
    assert(n >= 0);
    // Use a 'release store' so that anybody who reads through this
    // pointer observes a fully initialized version of the inserted node.
    next_[n].store(x, std::memory_order_release);
  }

  // No-barrier variants that can be safely used in a few locations.
  Node* NoBarrier_Next(int n) {
    assert(n >= 0);
    return next_[n].load(std::memory_order_relaxed);
  }
  void NoBarrier_SetNext(int n, Node* x) {
    assert(n >= 0);
    next_[n].store(x, std::memory_order_relaxed);
  }

 private:
  // Array of length equal to the node height.  next_[0] is lowest level link.
  // 长度为节点高度的原子指针数组，用于存储节点在不同层级上的后继节点。
  std::atomic<Node*> next_[1]; 
};

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::NewNode(
    const Key& key, int height) {
      // 用于分配一块内存，以容纳新节点的数据和指针数组。
      // sizeof(node) + sizeof(std::atomic<Node*> * (height - 1))
      // 将地址的起始指针赋给node_memory，声明为常量指针。
  char* const node_memory = arena_->AllocateAligned(
      sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1));
  // 返回指向新节点的指针
  // 定位new的方式，是对new的重载，用于在已经分配的内存上构造对象。
  return new (node_memory) Node(key);
}

template <typename Key, class Comparator>
// 初始化迭代器对象，设置初始状态。
inline SkipList<Key, Comparator>::Iterator::Iterator(const SkipList* list) {
  list_ = list; //list_是迭代器的成员变量
  node_ = nullptr; // node_设置为空，表示当前迭代器没有指向任何节点。
}

//valid()方法实现：判断迭代器是否处于有效的状态。
template <typename Key, class Comparator>
inline bool SkipList<Key, Comparator>::Iterator::Valid() const {
  return node_ != nullptr;
}

// 获取当前节点指向的键值
template <typename Key, class Comparator>
inline const Key& SkipList<Key, Comparator>::Iterator::key() const {
  assert(Valid());
  // 返回迭代器当前指向节点的键值。
  return node_->key;
}

// 将迭代器的成员变量 node_ 更新为当前节点的下一个节点。
template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Next() {
  assert(Valid());
  // 返回当前节点在第0层的后继节点，即下一个节点的指针。实现将迭代器向前移动到下一个节点的位置。
  node_ = node_->Next(0);
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Prev() {
  // Instead of using explicit "prev" links, we just search for the
  // last node that falls before key.
  assert(Valid());
  // findlessthan方法，搜索小于当前节点键值的最后一个节点。
  node_ = list_->FindLessThan(node_->key);
  // 如果指向的是跳表的头节点，则将node_更新为nullptr，表示迭代器没有指向任何节点。
  if (node_ == list_->head_) {
    // 这是为了处理特殊情况，如果当前节点是头节点，说明已经到达跳跃表的起始位置，因此将迭代器置为无效状态。
    node_ = nullptr;
  }
}

// 将skiplist迭代器定位到跳表中第一个大于或等于目标关键字的节点。
template <typename Key, class Comparator>
// skiplist类中的iterator类的seek函数的定义
inline void SkipList<Key, Comparator>::Iterator::Seek(const Key& target) {
  node_ = list_->FindGreaterOrEqual(target, nullptr);
}

template <typename Key, class Comparator>
// 这行代码将迭代器的node_成员变量设置为跳表的头节点的下一个节点（索引为0）。
// 换句话说，它将迭代器指向跳表中的第一个有效节点。
// 通过访问跳表的头节点，我们可以获取指向第一个节点的指针，并将其赋值给迭代器的node_成员变量，
// 从而完成将迭代器指向第一个节点的操作。
inline void SkipList<Key, Comparator>::Iterator::SeekToFirst() {
  node_ = list_->head_->Next(0);
}

// 将迭代器指向跳表的最后一个节点
template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SeekToLast() {
  node_ = list_->FindLast();
  if (node_ == list_->head_) {
    // 表示不指向任何有效节点
    node_ = nullptr;
  }
}

// 生成一个随机的高度值，用于确定新节点在跳表中的层数。
template <typename Key, class Comparator>
int SkipList<Key, Comparator>::RandomHeight() {
  // Increase height with probability 1 in kBranching
  // 声明一个常量kBranching,值为4,用于控制高度增加的概率。每当一个新节点的高度增加时，有1/4的概率增加一层。
  static const unsigned int kBranching = 4;
  // 新节点的初始高度为1
  int height = 1;
  while (height < kMaxHeight && rnd_.OneIn(kBranching)) {
    height++;
  }

  // 保证高度在有效范围内。
  assert(height > 0);
  assert(height <= kMaxHeight);
  return height;
}

template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::KeyIsAfterNode(const Key& key, Node* n) const {
  // null n is considered infinite
  return (n != nullptr) && (compare_(n->key, key) < 0);
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::FindGreaterOrEqual(const Key& key,
                                              Node** prev) const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (KeyIsAfterNode(key, next)) {
      // Keep searching in this list
      x = next;
    } else {
      if (prev != nullptr) prev[level] = x;
      if (level == 0) {
        return next;
      } else {
        // Switch to next list
        level--;
      }
    }
  }
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::FindLessThan(const Key& key) const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    assert(x == head_ || compare_(x->key, key) < 0);
    Node* next = x->Next(level);
    if (next == nullptr || compare_(next->key, key) >= 0) {
      if (level == 0) {
        return x;
      } else {
        // Switch to next list
        level--;
      }
    } else {
      x = next;
    }
  }
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::FindLast()
    const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (next == nullptr) {
      if (level == 0) {
        return x;
      } else {
        // Switch to next list
        level--;
      }
    } else {
      x = next;
    }
  }
}

template <typename Key, class Comparator>
// 跳表构造函数的实现
SkipList<Key, Comparator>::SkipList(Comparator cmp, Arena* arena)
    : compare_(cmp),
      arena_(arena),
      head_(NewNode(0 /* any key will do */, kMaxHeight)),
      max_height_(1),
      rnd_(0xdeadbeef) {
  for (int i = 0; i < kMaxHeight; i++) {
    head_->SetNext(i, nullptr);
  }
}

// 跳表的insert实现
template <typename Key, class Comparator>
void SkipList<Key, Comparator>::Insert(const Key& key) {
  // TODO(opt): We can use a barrier-free variant of FindGreaterOrEqual()
  // here since Insert() is externally synchronized.
  Node* prev[kMaxHeight];
  // 找到大于等于给定键值key的最早节点，并使用prev数组记录每个层级上的前一个节点。
  Node* x = FindGreaterOrEqual(key, prev);

  // Our data structure does not allow duplicate insertion
  assert(x == nullptr || !Equal(key, x->key));
  // 生成一个随机的节点高度，根据需要更新跳表的最大高度
  int height = RandomHeight();
  if (height > GetMaxHeight()) {
    for (int i = GetMaxHeight(); i < height; i++) {
      prev[i] = head_;
    }
    // It is ok to mutate max_height_ without any synchronization
    // with concurrent readers.  A concurrent reader that observes
    // the new value of max_height_ will see either the old value of
    // new level pointers from head_ (nullptr), or a new value set in
    // the loop below.  In the former case the reader will
    // immediately drop to the next level since nullptr sorts after all
    // keys.  In the latter case the reader will use the new node.
    max_height_.store(height, std::memory_order_relaxed);
  }
  // 然后，代码创建一个新的节点x，并为每个层级将节点x插入到跳跃表中。
  // 在插入过程中，通过更新节点的next指针来维护节点之间的链接关系。
  x = NewNode(key, height);
  for (int i = 0; i < height; i++) {
    // NoBarrier_SetNext() suffices since we will add a barrier when
    // we publish a pointer to "x" in prev[i].
    x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
    prev[i]->SetNext(i, x);
  }
}

template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::Contains(const Key& key) const {
  Node* x = FindGreaterOrEqual(key, nullptr);
  if (x != nullptr && Equal(key, x->key)) {
    return true;
  } else {
    return false;
  }
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_SKIPLIST_H_
