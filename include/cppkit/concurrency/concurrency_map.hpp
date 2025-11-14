#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <vector>
#include <mutex>
#include <cassert>

namespace cppkit::concurrency
{
  template <typename K, typename V>
  class ConcurrentHashMap
  {
    struct Node
    {
      K key;                            // 键
      V value;                          // 值
      std::atomic<Node*> next{nullptr}; // 原子指针用于无锁读取
      Node* next_plain{nullptr};        // 用于链表复制时的普通指针

      Node(const K& k, const V& v, Node* n = nullptr) : key(k), value(v), next(n), next_plain(nullptr) {}
    };

    struct Bucket
    {
      std::atomic<Node*> head{nullptr};  // 链表头指针
      mutable std::mutex mutex;          // 保护链表修改的互斥锁
      std::atomic<bool> migrated{false}; // 标记是否已迁移
    };

    struct Table
    {
      std::vector<Bucket> buckets;
      std::atomic<size_t> size{0};
      explicit Table(size_t n) : buckets(n) {}
    };

    struct RetiredNode
    {
      Node* node;
      size_t retire_epoch;
    };
    struct ThreadEpoch
    {
      std::atomic<size_t> epoch{0};
      std::vector<RetiredNode> retired;
    };

    std::atomic<size_t> global_epoch_{1};
    std::mutex threads_mutex_;
    std::vector<ThreadEpoch*> threads_;

    ThreadEpoch* get_thread_epoch()
    {
      thread_local ThreadEpoch local;
      thread_local bool registered = false;
      if (!registered)
      {
        std::lock_guard<std::mutex> lg(threads_mutex_);
        threads_.push_back(&local);
        registered = true;
      }
      return &local;
    }

    void enter_epoch()
    {
      ThreadEpoch* te = get_thread_epoch();
      size_t e = global_epoch_.load(std::memory_order_acquire);
      te->epoch.store(e, std::memory_order_release);
    }

    void leave_epoch()
    {
      ThreadEpoch* te = get_thread_epoch();
      te->epoch.store(0, std::memory_order_release);
    }

    void retire_node(Node* n)
    {
      ThreadEpoch* te = get_thread_epoch();
      size_t e = global_epoch_.load(std::memory_order_acquire);
      te->retired.push_back({n, e});
      if (te->retired.size() >= 64)
        try_reclaim(te);
    }

    void try_reclaim(ThreadEpoch* te)
    {
      global_epoch_.fetch_add(1, std::memory_order_acq_rel);
      size_t safe_epoch = global_epoch_.load(std::memory_order_acquire) - 1;

      size_t min_epoch = SIZE_MAX;
      {
        std::lock_guard<std::mutex> lg(threads_mutex_);
        for (auto t : threads_)
        {
          size_t ev = t->epoch.load(std::memory_order_acquire);
          if (ev == 0)
            continue;
          if (ev < min_epoch)
            min_epoch = ev;
        }
      }
      if (min_epoch == SIZE_MAX)
        min_epoch = safe_epoch;

      auto& rvec = te->retired;
      size_t i = 0;
      for (size_t j = 0; j < rvec.size(); ++j)
      {
        if (rvec[j].retire_epoch < min_epoch)
          delete rvec[j].node;
        else
          rvec[i++] = rvec[j];
      }
      rvec.resize(i);
    }
    // ------------ end epoch reclamation ------------

    std::shared_ptr<Table> table_;
    std::shared_ptr<Table> rehash_table_;
    std::atomic<size_t> rehash_index_{0};
    mutable std::mutex table_mutex_;
    static constexpr double LOAD_FACTOR = 0.75;
    static constexpr size_t MIGRATE_BATCH = 1;

  public:
    explicit ConcurrentHashMap(size_t initialBuckets = 16) { table_ = std::make_shared<Table>(initialBuckets); }

    ~ConcurrentHashMap()
    {
      auto t = table_;
      for (auto& b : t->buckets)
      {
        Node* n = b.head.load(std::memory_order_acquire);
        while (n)
        {
          Node* next = n->next.load(std::memory_order_acquire);
          delete n;
          n = next;
        }
      }
      if (rehash_table_)
      {
        auto r = rehash_table_;
        for (auto& b : r->buckets)
        {
          Node* n = b.head.load(std::memory_order_acquire);
          while (n)
          {
            Node* next = n->next.load(std::memory_order_acquire);
            delete n;
            n = next;
          }
        }
      }
    }

    std::optional<V> get(const K& key)
    {
      enter_epoch();
      auto t = table_;
      size_t idx = bucketIndex(t, key);
      Node* n = t->buckets[idx].head.load(std::memory_order_acquire);
      while (n)
      {
        if (n->key == key)
        {
          V val = n->value;
          leave_epoch();
          return val;
        }
        n = n->next.load(std::memory_order_acquire);
      }

      if (rehash_table_)
      {
        auto r = rehash_table_;
        size_t idx2 = std::hash<K>{}(key) % r->buckets.size();
        n = r->buckets[idx2].head.load(std::memory_order_acquire);
        while (n)
        {
          if (n->key == key)
          {
            V val = n->value;
            leave_epoch();
            return val;
          }
          n = n->next.load(std::memory_order_acquire);
        }
      }
      leave_epoch();
      return std::nullopt;
    }

    void put(const K& key, const V& value)
    {
      helpMigrateSome();

      while (true)
      {
        auto t = table_;
        size_t idx = bucketIndex(t, key);
        Bucket& b = t->buckets[idx];

        Node* head = b.head.load(std::memory_order_acquire);
        Node* n = head;
        bool found = false;
        while (n)
        {
          if (n->key == key)
          {
            found = true;
            break;
          }
          n = n->next.load(std::memory_order_acquire);
        }

        if (found)
        {
          std::unique_lock<std::mutex> lg(b.mutex);
          head = b.head.load(std::memory_order_acquire);
          n = head;
          Node* newHead = nullptr;
          Node* tail = nullptr;
          while (n)
          {
            Node* nn;
            if (n->key == key)
              nn = new Node(key, value);
            else
              nn = new Node(n->key, n->value);

            if (!newHead)
              newHead = nn;
            if (tail)
              tail->next_plain = nn;
            tail = nn;
            n = n->next.load(std::memory_order_acquire);
          }

          // 将普通链表发布到原子 head
          Node* cur = newHead;
          while (cur)
          {
            Node* next = cur->next_plain;
            cur->next.store(next, std::memory_order_release);
            cur = next;
          }

          b.head.store(newHead, std::memory_order_release);
          retire_list(head);
          return;
        }
        else
        {
          Node* oldHead = head;
          Node* newNode = new Node(key, value, oldHead);
          if (b.head.compare_exchange_weak(oldHead, newNode, std::memory_order_release, std::memory_order_relaxed))
          {
            t->size.fetch_add(1, std::memory_order_relaxed);
            if ((double) t->size.load(std::memory_order_relaxed) / t->buckets.size() > LOAD_FACTOR)
              startRehash();
            return;
          }
          delete newNode;
          std::this_thread::yield();
        }
      }
    }

    bool erase(const K& key)
    {
      helpMigrateSome();
      auto t = table_;
      size_t idx = bucketIndex(t, key);
      Bucket& b = t->buckets[idx];
      std::unique_lock<std::mutex> lg(b.mutex);
      Node* head = b.head.load(std::memory_order_acquire);
      Node* cur = head;
      Node* newHead = nullptr;
      Node* tail = nullptr;
      bool removed = false;

      while (cur)
      {
        if (!removed && cur->key == key)
          removed = true;
        else
        {
          Node* nn = new Node(cur->key, cur->value);
          if (!newHead)
            newHead = nn;
          if (tail)
            tail->next_plain = nn;
          tail = nn;
        }
        cur = cur->next.load(std::memory_order_acquire);
      }
      if (removed)
      {
        Node* cur = newHead;
        while (cur)
        {
          Node* next = cur->next_plain;
          cur->next.store(next, std::memory_order_release);
          cur = next;
        }
        b.head.store(newHead, std::memory_order_release);
        t->size.fetch_sub(1, std::memory_order_relaxed);
        retire_list(head);
        return true;
      }
      return false;
    }

    size_t size() const { return table_->size.load(std::memory_order_relaxed); }

  private:
    void retire_list(Node* head)
    {
      Node* n = head;
      while (n)
      {
        Node* next = n->next.load(std::memory_order_acquire);
        retire_node(n);
        n = next;
      }
    }

    void startRehash()
    {
      std::unique_lock<std::mutex> lg(table_mutex_);
      if (rehash_table_)
        return;
      auto old = table_;
      rehash_table_ = std::make_shared<Table>(old->buckets.size() * 2);
      rehash_index_.store(0, std::memory_order_release);
    }

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
          std::unique_lock<std::mutex> lg(table_mutex_);
          if (rehash_table_ && rehash_index_.load(std::memory_order_acquire) >= old->buckets.size())
          {
            for (size_t j = 0; j < old->buckets.size(); ++j)
            {
              if (!old->buckets[j].migrated.load(std::memory_order_acquire))
              {
                std::unique_lock<std::mutex> blg(old->buckets[j].mutex);
                Node* n = old->buckets[j].head.load(std::memory_order_acquire);
                while (n)
                {
                  size_t newIdx = std::hash<K>{}(n->key) % newt->buckets.size();
                  Bucket& nb = newt->buckets[newIdx];
                  Node* oldHead = nb.head.load(std::memory_order_acquire);
                  Node* copy = new Node(n->key, n->value, oldHead);
                  while (!nb.head.compare_exchange_weak(
                      oldHead, copy, std::memory_order_release, std::memory_order_relaxed))
                  {
                  }
                  newt->size.fetch_add(1, std::memory_order_relaxed);
                  n = n->next.load(std::memory_order_acquire);
                }
                old->buckets[j].head.store(nullptr, std::memory_order_release);
                old->buckets[j].migrated.store(true, std::memory_order_release);
              }
            }
            table_.swap(rehash_table_);
            rehash_table_.reset();
            rehash_index_.store(0, std::memory_order_release);
          }
          return;
        }

        Bucket& ob = old->buckets[idx];
        std::unique_lock<std::mutex> blg(ob.mutex);
        if (ob.migrated.load(std::memory_order_acquire))
          continue;

        Node* n = ob.head.load(std::memory_order_acquire);
        while (n)
        {
          size_t newIdx = std::hash<K>{}(n->key) % newt->buckets.size();
          Bucket& nb = newt->buckets[newIdx];
          Node* oldHead = nb.head.load(std::memory_order_acquire);
          Node* copy = new Node(n->key, n->value, oldHead);
          while (!nb.head.compare_exchange_weak(oldHead, copy, std::memory_order_release, std::memory_order_relaxed))
          {
          }
          newt->size.fetch_add(1, std::memory_order_relaxed);
          n = n->next.load(std::memory_order_acquire);
        }
        ob.head.store(nullptr, std::memory_order_release);
        ob.migrated.store(true, std::memory_order_release);
      }
    }

    static size_t bucketIndex(const std::shared_ptr<Table>& t, const K& key)
    {
      return std::hash<K>{}(key) % t->buckets.size();
    }

  public:
    // ---------------- iterable ----------------
    struct Entry
    {
      K key;
      V value;
    };

    class Iterator
    {
      std::shared_ptr<std::vector<Entry>> snapshot_;
      size_t pos_ = 0;

    public:
      Iterator(std::shared_ptr<std::vector<Entry>> snap, size_t pos = 0) : snapshot_(snap), pos_(pos) {}
      Entry& operator*() { return (*snapshot_)[pos_]; }
      Entry* operator->() { return &(*snapshot_)[pos_]; }
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
      bool operator==(const Iterator& o) const { return snapshot_ == o.snapshot_ && pos_ == o.pos_; }
      bool operator!=(const Iterator& o) const { return !(*this == o); }
    };

    class Iterable
    {
      std::shared_ptr<std::vector<Entry>> snap_;

    public:
      explicit Iterable(std::shared_ptr<std::vector<Entry>> snap) : snap_(snap) {}
      Iterator begin() { return Iterator(snap_, 0); }
      Iterator end() { return Iterator(snap_, snap_->size()); }
    };

    Iterable iterable() const
    {
      auto snap = std::make_shared<std::vector<Entry>>();
      auto t = table_;
      auto r = rehash_table_;
      for (auto& b : t->buckets)
      {
        Node* n = b.head.load(std::memory_order_acquire);
        while (n)
        {
          snap->push_back({n->key, n->value});
          n = n->next.load(std::memory_order_acquire);
        }
      }
      if (r)
      {
        for (auto& b : r->buckets)
        {
          Node* n = b.head.load(std::memory_order_acquire);
          while (n)
          {
            snap->push_back({n->key, n->value});
            n = n->next.load(std::memory_order_acquire);
          }
        }
      }
      return Iterable(snap);
    }
  };
} // namespace cppkit::concurrency
