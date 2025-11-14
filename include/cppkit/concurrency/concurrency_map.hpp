#pragma once

#include <vector>
#include <mutex>
#include <optional>
#include <memory>
#include <atomic>
#include <functional>
#include <thread>

namespace cppkit::concurrency
{

  template <typename K, typename V>
  class ConcurrentHashMap
  {
    struct Node
    {
      K key;                      // 键
      V value;                    // 值
      std::shared_ptr<Node> next; // 下一个节点

      Node(K k, V v, std::shared_ptr<Node> n = nullptr) : key(std::move(k)), value(std::move(v)), next(std::move(n)) {}
    };

    struct Bucket
    {
      // 链表头节点
      std::shared_ptr<Node> head;        // 头节点
      mutable std::mutex mutex;          // 保护写入
      std::atomic<bool> migrated{false}; // 是否已迁移到新表
    };

    struct Table
    {
      std::vector<Bucket> buckets; // 桶数组
      std::atomic<size_t> size{0}; // 当前元素数量

      explicit Table(size_t n) : buckets(n) {}
    };

    std::shared_ptr<Table> table_;        // 当前哈希表
    std::shared_ptr<Table> rehash_table_; // 重新哈希表（调整大小时使用的新表）

    std::atomic<size_t> rehash_index_{0}; // 重新哈希进度索引

    mutable std::mutex table_mutex_; // 保护启动/完成重新哈希和交换
    static constexpr double LOAD_FACTOR = 0.75;
    static constexpr size_t MIGRATE_BATCH = 1; // 每次帮助调用迁移多少个桶

  public:
    explicit ConcurrentHashMap(const size_t initialBuckets = 16) { table_ = std::make_shared<Table>(initialBuckets); }

    // 读取操作无需锁定桶，使用原子加载头节点并遍历链表
    std::optional<V> get(const K& key)
    {
      helpMigrateSome();

      while (true)
      {
        auto t = table_;
        const size_t idx = bucketIndex(t, key);
        Bucket& bucket = t->buckets[idx];

        // 快路径：原子加载头节点，无需加锁
        std::shared_ptr<Node> node = std::atomic_load(&bucket.head);
        // 如果桶已迁移，重试（将使用新表）
        if (bucket.migrated.load(std::memory_order_acquire))
        {
          helpMigrateSome();
          continue;
        }

        for (; node; node = std::atomic_load(&node->next))
        {
          if (node->key == key)
            return node->value;
        }

        // 未找到，检查新表
        auto r = rehash_table_;
        if (r)
        {
          const size_t idx2 = std::hash<K>{}(key) % r->buckets.size();
          Bucket& nb = r->buckets[idx2];
          std::shared_ptr<Node> nnode = std::atomic_load(&nb.head);
          for (; nnode; nnode = std::atomic_load(&nnode->next))
            if (nnode->key == key)
              return nnode->value;
        }

        return std::nullopt;
      }
    }

    // 写操作获取桶的互斥锁，但使用 atomic_store 发布新的头节点，以便读取者可以无锁遍历
    void put(const K& key, const V& value)
    {
      helpMigrateSome();

      while (true)
      {
        auto t = table_; // 当前表的快照
        const size_t idx = bucketIndex(t, key);
        Bucket& bucket = t->buckets[idx];

        // 如果这个桶已经迁移，重试使用新表
        if (bucket.migrated.load(std::memory_order_acquire))
        {
          helpMigrateSome();
          continue;
        }

        {
          std::unique_lock lock(bucket.mutex);

          // 获取锁后再次检查
          if (bucket.migrated.load(std::memory_order_acquire))
          {
            continue;
          }

          // 检查是否存在键，更新值
          std::shared_ptr<Node> head = std::atomic_load(&bucket.head);
          for (auto node = head; node; node = std::atomic_load(&node->next))
          {
            if (node->key == key)
            {
              // 为了对任意类型的 V 都严格安全，替换节点而不是就地修改值。
              // 我们将通过创建具有相同下一个指针的新节点来替换节点。
              auto newNode = std::make_shared<Node>(key, value, std::atomic_load(&node->next));

              // 将 newNode 链接到 node 的位置。最简单的方法是重建头部列表直到达到 node。
              // 通过持有桶的互斥锁，可以安全地重建。
              std::shared_ptr<Node> cur = head;
              std::shared_ptr<Node> newHead = nullptr;
              std::shared_ptr<Node> tail = nullptr;

              while (cur)
              {
                if (cur.get() == node.get())
                {
                  if (!tail)
                  {
                    // node 是头节点
                    newHead = newNode;
                  }
                  else
                  {
                    tail->next = newNode;
                  }
                  newNode->next = std::atomic_load(&cur->next);
                }
                else
                {
                  auto copy = std::make_shared<Node>(cur->key, cur->value, nullptr);
                  if (!newHead)
                    newHead = copy;
                  else
                    tail->next = copy;
                  copy->next = nullptr;
                  tail = copy;
                }
                cur = std::atomic_load(&cur->next);
              }
              // 原子发布新的头节点，以便读取者可以看到旧头节点或新头节点
              std::atomic_store(&bucket.head, newHead);
              return;
            }
          }

          // 键不存在，插入新节点到头部
          auto curHead = std::atomic_load(&bucket.head);
          auto newNode = std::make_shared<Node>(key, value, curHead);
          // 原子发布新的头节点，以便读取者可以看到旧头节点或新头节点
          std::atomic_store(&bucket.head, newNode);
          t->size.fetch_add(1, std::memory_order_relaxed);
        }

        // 如果负载因子超过阈值，可能开始重新哈希
        if ((double) t->size.load(std::memory_order_relaxed) / t->buckets.size() > LOAD_FACTOR)
        {
          startRehash();
        }
        return;
      }
    }

    bool erase(const K& key)
    {
      helpMigrateSome();

      while (true)
      {
        auto t = table_;
        const size_t idx = bucketIndex(t, key);
        Bucket& bucket = t->buckets[idx];

        if (bucket.migrated.load(std::memory_order_acquire))
        {
          helpMigrateSome();
          continue; // 尝试新表
        }

        {
          std::unique_lock lock(bucket.mutex);
          if (bucket.migrated.load(std::memory_order_acquire))
            continue;

          // 重新构建链表，跳过被删除的节点；使用 shared_ptr 快照以保证安全
          std::shared_ptr<Node> head = std::atomic_load(&bucket.head);
          std::shared_ptr<Node> cur = head;
          std::shared_ptr<Node> newHead = nullptr;
          std::shared_ptr<Node> tail = nullptr;
          bool removed = false;

          while (cur)
          {
            if (!removed && cur->key == key)
            {
              // 跳过该节点
              removed = true;
            }
            else
            {
              auto copy = std::make_shared<Node>(cur->key, cur->value, nullptr);
              if (!newHead)
                newHead = copy;
              else
                tail->next = copy;
              tail = copy;
            }
            cur = std::atomic_load(&cur->next);
          }

          if (removed)
          {
            std::atomic_store(&bucket.head, newHead);
            t->size.fetch_sub(1, std::memory_order_relaxed);
            return true;
          }
        }

        // 找不到，检查新表
        auto r = rehash_table_;
        if (r)
        {
          const size_t idx2 = std::hash<K>{}(key) % r->buckets.size();
          Bucket& nb = r->buckets[idx2];
          std::unique_lock lock2(nb.mutex);
          std::shared_ptr<Node> head = std::atomic_load(&nb.head);
          std::shared_ptr<Node> cur = head;
          std::shared_ptr<Node> newHead = nullptr;
          std::shared_ptr<Node> tail = nullptr;
          bool removed = false;

          while (cur)
          {
            if (!removed && cur->key == key)
            {
              removed = true;
            }
            else
            {
              auto copy = std::make_shared<Node>(cur->key, cur->value, nullptr);
              if (!newHead)
                newHead = copy;
              else
                tail->next = copy;
              tail = copy;
            }
            cur = std::atomic_load(&cur->next);
          }

          if (removed)
          {
            std::atomic_store(&nb.head, newHead);
            r->size.fetch_sub(1, std::memory_order_relaxed);
            return true;
          }
        }

        return false;
      }
    }

    size_t size() const
    {
      auto t = table_;
      auto r = rehash_table_;
      if (!r)
        return t->size.load(std::memory_order_relaxed);
      // 求和 非迁移的旧桶 + 新表
      size_t s = r->size.load(std::memory_order_relaxed);
      for (size_t i = 0; i < t->buckets.size(); ++i)
      {
        if (!t->buckets[i].migrated.load(std::memory_order_acquire))
        {
          // 计数旧桶中的节点
          std::shared_ptr<Node> node = std::atomic_load(&t->buckets[i].head);
          while (node)
          {
            ++s;
            node = std::atomic_load(&node->next);
          }
        }
      }
      return s;
    }

  private:
    // 开始重新哈希
    void startRehash()
    {
      std::unique_lock lock(table_mutex_);
      if (rehash_table_)
        return;
      auto oldTable = table_;
      rehash_table_ = std::make_shared<Table>(oldTable->buckets.size() * 2);
      rehash_index_.store(0, std::memory_order_release);
    }

    // 帮助迁移一些桶
    void helpMigrateSome()
    {
      auto old = table_;
      auto newt = rehash_table_;
      if (!newt)
        return;

      for (size_t i = 0; i < MIGRATE_BATCH; ++i)
      {
        size_t idx = rehash_index_.fetch_add(1, std::memory_order_acq_rel);
        if (idx >= old->buckets.size())
        {
          // 检查是否完成
          std::unique_lock lock(table_mutex_);
          if (rehash_table_ && rehash_index_.load(std::memory_order_acquire) >= old->buckets.size())
          {
            // 完成任何剩余的迁移
            for (size_t j = 0; j < old->buckets.size(); ++j)
            {
              if (!old->buckets[j].migrated.load(std::memory_order_acquire))
              {
                std::unique_lock oldLock(old->buckets[j].mutex);
                if (old->buckets[j].head)
                {
                  std::shared_ptr<Node> node = std::atomic_load(&old->buckets[j].head);
                  while (node)
                  {
                    size_t newIdx = std::hash<K>{}(node->key) % newt->buckets.size();
                    Bucket& newBucket = newt->buckets[newIdx];
                    std::unique_lock newLock(newBucket.mutex);
                    auto curHead = std::atomic_load(&newBucket.head);
                    auto copy = std::make_shared<Node>(node->key, node->value, curHead);
                    std::atomic_store(&newBucket.head, copy);
                    newt->size.fetch_add(1, std::memory_order_relaxed);
                    node = std::atomic_load(&node->next);
                  }
                }
                old->buckets[j].head = nullptr;
                old->buckets[j].migrated.store(true, std::memory_order_release);
              }
            }

            // 交换表指针
            table_.swap(rehash_table_);
            rehash_table_.reset();
            rehash_index_.store(0, std::memory_order_release);
          }
          return;
        }

        Bucket& oldBucket = old->buckets[idx];
        // 尝试锁定旧桶并移动节点；之后标记为已迁移
        {
          std::unique_lock oldLock(oldBucket.mutex);
          if (oldBucket.migrated.load(std::memory_order_acquire))
            continue;

          std::shared_ptr<Node> node = std::atomic_load(&oldBucket.head);
          while (node)
          {
            size_t newIdx = std::hash<K>{}(node->key) % newt->buckets.size();
            Bucket& newBucket = newt->buckets[newIdx];
            std::unique_lock newLock(newBucket.mutex);
            auto curHead = std::atomic_load(&newBucket.head);
            auto copy = std::make_shared<Node>(node->key, node->value, curHead);
            std::atomic_store(&newBucket.head, copy);
            newt->size.fetch_add(1, std::memory_order_relaxed);
            node = std::atomic_load(&node->next);
          }
          oldBucket.head = nullptr;
          oldBucket.migrated.store(true, std::memory_order_release);
        }
      }
    }

    static size_t bucketIndex(const std::shared_ptr<Table>& t, const K& key)
    {
      return std::hash<K>{}(key) % t->buckets.size();
    }

  public:
    struct Entry
    {
      K key;
      V value;
    };

    // 快照迭代器
    // 从未迁移的旧桶和新表中收集Entry
    class Iterator
    {
    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = Entry;
      using difference_type = std::ptrdiff_t;
      using pointer = value_type*;
      using reference = value_type&;

      explicit Iterator(std::shared_ptr<std::vector<Entry>> snap, size_t pos = 0) : snapshot_(snap), pos_(pos) {}

      reference operator*() { return (*snapshot_)[pos_]; }
      pointer operator->() { return &(*snapshot_)[pos_]; }

      Iterator& operator++()
      {
        ++pos_;
        return *this;
      }

      Iterator operator++(int)
      {
        Iterator tmp = *this;
        ++(*this);
        return tmp;
      }

      bool operator==(const Iterator& other) const { return snapshot_ == other.snapshot_ && pos_ == other.pos_; }
      bool operator!=(const Iterator& other) const { return !(*this == other); }

    private:
      std::shared_ptr<std::vector<Entry>> snapshot_;
      size_t pos_;
    };

    class Iterable
    {
    public:
      explicit Iterable(std::shared_ptr<std::vector<Entry>> snap) : snap_(snap) {}

      Iterator begin() { return Iterator(snap_, 0); }
      Iterator end() { return Iterator(snap_, snap_->size()); }

    private:
      std::shared_ptr<std::vector<Entry>> snap_;
    };

    Iterable iterable() const
    {
      auto snap = snapshot();
      return Iterable(snap);
    }

  private:
    std::shared_ptr<std::vector<Entry>> snapshot() const
    {
      auto snap = std::make_shared<std::vector<Entry>>();
      auto t = table_;
      auto r = rehash_table_;

      // 从旧表中获取未迁移的桶
      for (size_t i = 0; i < t->buckets.size(); ++i)
      {
        if (t->buckets[i].migrated.load(std::memory_order_acquire))
          continue;
        std::shared_ptr<Node> node = std::atomic_load(&t->buckets[i].head);
        while (node)
        {
          snap->push_back({node->key, node->value});
          node = std::atomic_load(&node->next);
        }
      }

      // 从新表中获取
      if (r)
      {
        for (size_t i = 0; i < r->buckets.size(); ++i)
        {
          std::shared_ptr<Node> node = std::atomic_load(&r->buckets[i].head);
          while (node)
          {
            snap->push_back({node->key, node->value});
            node = std::atomic_load(&node->next);
          }
        }
      }

      return snap;
    }
  };
} // namespace cppkit::concurrency
