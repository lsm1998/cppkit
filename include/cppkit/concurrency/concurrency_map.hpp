#pragma once

#include <vector>
#include <mutex>
#include <optional>
#include <memory>
#include <atomic>
#include <functional>

namespace cppkit::concurrency
{
  template <typename K, typename V>
  class ConcurrentHashMap
  {
    struct Node
    {
      K key;
      V value;
      std::shared_ptr<Node> next;

      Node(K k, V v, std::shared_ptr<Node> n = nullptr)
        : key(std::move(k)), value(std::move(v)), next(std::move(n))
      {
      }
    };

    struct Bucket
    {
      std::shared_ptr<Node> head;
      mutable std::mutex mutex;
    };

    struct Table
    {
      std::vector<Bucket> buckets;
      std::atomic<size_t> size{0};

      explicit Table(size_t n) : buckets(n)
      {
      }
    };

    std::shared_ptr<Table> table_;
    std::shared_ptr<Table> rehash_table_; // 正在迁移的新表
    mutable std::mutex table_mutex_;
    static constexpr double LOAD_FACTOR = 0.75;

  public:
    explicit ConcurrentHashMap(const size_t initialBuckets = 16)
    {
      table_ = std::make_shared<Table>(initialBuckets);
    }

    void put(const K& key, const V& value)
    {
      migrateSomeBuckets();

      auto t = table_;
      auto& bucket = t->buckets[std::hash<K>{}(key) % t->buckets.size()];
      std::unique_lock lock(bucket.mutex);
      for (auto node = bucket.head; node; node = node->next)
      {
        if (node->key == key)
        {
          node->value = value;
          return;
        }
      }
      bucket.head = std::make_shared<Node>(key, value, bucket.head);
      ++t->size;

      if ((double) t->size.load() / t->buckets.size() > LOAD_FACTOR)
      {
        startRehash();
      }
    }

    std::optional<V> get(const K& key)
    {
      migrateSomeBuckets();

      auto t = table_;
      auto& bucket = t->buckets[std::hash<K>{}(key) % t->buckets.size()];
      std::unique_lock lock(bucket.mutex);
      for (auto node = bucket.head; node; node = node->next)
      {
        if (node->key == key)
          return node->value;
      }

      if (rehash_table_)
      {
        auto& newBucket = rehash_table_->buckets[std::hash<K>{}(key) % rehash_table_->buckets.size()];
        std::unique_lock lock2(newBucket.mutex);
        for (auto node = newBucket.head; node; node = node->next)
        {
          if (node->key == key)
            return node->value;
        }
      }
      return std::nullopt;
    }

    bool erase(const K& key)
    {
      migrateSomeBuckets();

      auto t = table_;
      auto& bucket = t->buckets[std::hash<K>{}(key) % t->buckets.size()];
      std::unique_lock lock(bucket.mutex);
      std::shared_ptr<Node> prev;
      for (auto node = bucket.head; node; node = node->next)
      {
        if (node->key == key)
        {
          if (prev)
            prev->next = node->next;
          else
            bucket.head = node->next;
          --t->size;
          return true;
        }
        prev = node;
      }
      return false;
    }

    size_t size() const
    {
      return table_->size.load();
    }

  private:
    void startRehash()
    {
      std::unique_lock lock(table_mutex_);
      if (rehash_table_)
        return;
      auto oldTable = table_;
      rehash_table_ = std::make_shared<Table>(oldTable->buckets.size() * 2);
    }

    void migrateSomeBuckets()
    {
      std::unique_lock lock(table_mutex_);
      if (!rehash_table_)
        return;

      for (size_t i = 0; i < table_->buckets.size(); ++i)
      {
        auto& oldBucket = table_->buckets[i];
        std::unique_lock lockBucket(oldBucket.mutex);
        auto node = oldBucket.head;
        while (node)
        {
          auto& newBucket = rehash_table_->buckets[std::hash<K>{}(node->key) % rehash_table_->buckets.size()];
          std::unique_lock newLock(newBucket.mutex);
          newBucket.head = std::make_shared<Node>(node->key, node->value, newBucket.head);
          ++rehash_table_->size;
          node = node->next;
        }
        oldBucket.head = nullptr;
      }

      table_ = rehash_table_;
      rehash_table_ = nullptr;
    }

  public:
    struct Entry
    {
      K key;
      V value;
    };

    class Iterator
    {
    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = Entry;
      using difference_type = std::ptrdiff_t;
      using pointer = value_type*;
      using reference = value_type&;

      explicit Iterator(std::shared_ptr<std::vector<Entry>> snap, size_t pos = 0)
        : snapshot_(snap), pos_(pos)
      {
      }

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
      explicit Iterable(std::shared_ptr<std::vector<Entry>> snap) : snap_(snap)
      {
      }

      Iterator begin() { return Iterator(snap_, 0); }
      Iterator end() { return Iterator(snap_, snap_->size()); }

    private:
      std::shared_ptr<std::vector<Entry>> snap_;
    };

    Iterable iterable() const
    {
      auto snap = snapshot(); // 只生成一次 snapshot
      return Iterable(snap);
    }

  private:
    std::shared_ptr<std::vector<Entry>> snapshot() const
    {
      auto snap = std::make_shared<std::vector<Entry>>();
      for (auto& bucket : table_->buckets)
      {
        std::unique_lock lock(bucket.mutex);
        for (auto node = bucket.head; node; node = node->next)
          snap->push_back({node->key, node->value});
      }
      if (rehash_table_)
      {
        for (auto& bucket : rehash_table_->buckets)
        {
          std::unique_lock lock(bucket.mutex);
          for (auto node = bucket.head; node; node = node->next)
            snap->push_back({node->key, node->value});
        }
      }
      return snap;
    }
  };
} // namespace cppkit::concurrency