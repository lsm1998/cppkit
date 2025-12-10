#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>
#include <vector>

namespace cppkit::concurrency
{
  template <typename K, typename V>
  class SyncMap
  {
  private:
    // 标记 Entry 是否被从 dirty map 中清除
    struct Expunged
    {
    };

    struct Entry
    {
      using ValuePtr = std::shared_ptr<V>;
      std::atomic<ValuePtr> p;

      Entry() : p(nullptr) {}
      Entry(ValuePtr v) : p(v) {}

      // 获取 Expunged 标记的单例指针
      static ValuePtr get_expunged()
      {
        static auto ptr = std::make_shared<V>(); // 创建一个特殊的哨兵对象
        // 这里我们实际上只需要一个唯一的地址，但在强类型下，用一个真实对象最安全
        // 为了区分，我们在逻辑判断时比较指针地址，或者使用特定标记
        // 更好的方式是使用 nullptr 表示 nil，使用特殊指针表示 expunged
        return ptr;
      }

      // 核心加载逻辑
      // 返回值: optimal<V>，如果有值则返回，如果是 nil 或 expunged 则返回 nullopt
      std::shared_ptr<V> load()
      {
        auto ptr = p.load(std::memory_order_acquire);
        if (ptr == nullptr || ptr == get_expunged())
        {
          return nullptr;
        }
        return ptr;
      }

      // 尝试存储（只有当 entry 未被 expunged 时才成功）
      bool try_store(const std::shared_ptr<V>& i)
      {
        auto ptr = p.load(std::memory_order_acquire);
        while (ptr != get_expunged())
        {
          if (p.compare_exchange_weak(ptr, i, std::memory_order_release, std::memory_order_acquire))
          {
            return true;
          }
          // CAS 失败，ptr 会被更新为最新值，循环重试
        }
        return false;
      }

      // 无条件解雇 expunged 状态
      bool unexpunge_locked()
      {
        auto ptr = p.load(std::memory_order_acquire);
        return p.compare_exchange_strong(
            ptr, nullptr, std::memory_order_release, std::memory_order_acquire); // 如果是 expunged，置为 nil
      }

      // 交换值
      std::shared_ptr<V> swap_locked(const std::shared_ptr<V>& i) { return p.exchange(i, std::memory_order_acq_rel); }

      // 尝试标记为 expunged
      bool try_expunge_locked()
      {
        auto ptr = p.load(std::memory_order_acquire);
        while (ptr == nullptr)
        {
          if (p.compare_exchange_weak(ptr, get_expunged(), std::memory_order_release, std::memory_order_acquire))
          {
            return true;
          }
        }
        return false;
      }

      struct ReadOnly
      {
        std::unordered_map<K, std::shared_ptr<Entry>> m;
        bool amended = false; // 如果 dirty 中包含 read 中没有的 key，则为 true
      };
    };

  public:
    SyncMap()
    {
      // 初始化 read 为空的 map
      read_.store(std::make_shared<ReadOnly>());
    }

    std::pair<std::shared_ptr<V>, bool> Load(const K& key)
    {
      auto read = read_.load(std::memory_order_acquire);
      auto it = read->m.find(key);

      // 1. 尝试在 read map 中查找
      if (it == read->m.end() && read->amended)
      {
        // read 中没有，且 dirty 中可能有新数据，加锁升级处理
        std::lock_guard<std::mutex> lock(mu_);

        // Double-check locking
        read = read_.load(std::memory_order_acquire);
        it = read->m.find(key);

        if (it == read->m.end() && read->amended)
        {
          auto dit = dirty_.find(key);
          if (dit != dirty_.end())
          {
            auto val = dit->second->load();
            miss_locked(); // 记录一次 miss
            return {val, val != nullptr};
          }
          miss_locked();
          return {nullptr, false};
        }
      }

      if (it != read->m.end())
      {
        auto val = it->second->load();
        return {val, val != nullptr};
      }

      return {nullptr, false};
    }

    void Store(const K& key, const V& value)
    {
      auto val_ptr = std::make_shared<V>(value);
      auto read = read_.load(std::memory_order_acquire);
      auto it = read->m.find(key);

      // 1. 尝试直接更新 read map 中的 entry (Fast Path)
      if (it != read->m.end())
      {
        // 尝试无锁更新。如果 entry 是 expunged，try_store 会失败
        if (it->second->try_store(val_ptr))
        {
          return;
        }
      }

      // 2. Slow Path
      std::lock_guard<std::mutex> lock(mu_);
      read = read_.load(std::memory_order_acquire);
      it = read->m.find(key);

      if (it != read->m.end())
      {
        if (it->second->unexpunge_locked())
        {
          // Entry 之前被 expunged，意味着它不在 dirty 中
          // 需要将其加入 dirty
          dirty_[key] = it->second;
        }
        it->second->swap_locked(val_ptr);
      }
      else
      {
        auto dit = dirty_.find(key);
        if (dit != dirty_.end())
        {
          // 在 dirty 中找到了，直接更新
          dit->second->swap_locked(val_ptr);
        }
        else
        {
          // read 和 dirty 都没有
          if (!read->amended)
          {
            // 这是 dirty 第一次变脏，需要从 read 复制数据到 dirty
            dirty_locked();
            read_.store(std::make_shared<ReadOnly>(read->m, true), std::memory_order_release);
          }
          // 创建新 Entry 放入 dirty
          auto entry = std::make_shared<Entry>(val_ptr);
          dirty_[key] = entry;
        }
      }
    }

    std::pair<std::shared_ptr<V>, bool> LoadOrStore(const K& key, const V& value)
    {
      auto val_ptr = std::make_shared<V>(value);
      auto read = read_.load(std::memory_order_acquire);
      auto it = read->m.find(key);

      if (it != read->m.end())
      {
        auto actual = it->second->load();
        if (actual != nullptr)
          return {actual, true};
        // 可能是 nil 或 expunged，继续走 slow path
      }

      std::lock_guard<std::mutex> lock(mu_);
      read = read_.load(std::memory_order_acquire);
      it = read->m.find(key);

      if (it != read->m.end())
      {
        auto actual = it->second->load();
        if (it->second->unexpunge_locked())
        {
          dirty_[key] = it->second;
        }
        // 如果原本就有值(且不是expunged)，返回旧值
        // 这里的逻辑稍微复杂：unexpunge 把 expunged 变成了 nil
        // 所以我们需要再次判断 actual
        if (actual)
          return {actual, true};

        // 如果是 nil，将其设为新值
        it->second->swap_locked(val_ptr);
        return {val_ptr, false};
      }

      auto dit = dirty_.find(key);
      if (dit != dirty_.end())
      {
        auto actual = dit->second->load();
        if (actual)
          return {actual, true};
        dit->second->swap_locked(val_ptr);
        return {val_ptr, false};
      }

      if (!read->amended)
      {
        dirty_locked();
        read_.store(std::make_shared<ReadOnly>(read->m, true), std::memory_order_release);
      }
      auto entry = std::make_shared<Entry>(val_ptr);
      dirty_[key] = entry;
      return {val_ptr, false};
    }

    void Delete(const K& key) { LoadAndDelete(key); }

    std::shared_ptr<V> LoadAndDelete(const K& key)
    {
      auto read = read_.load(std::memory_order_acquire);
      auto it = read->m.find(key);

      if (it == read->m.end() && read->amended)
      {
        std::lock_guard<std::mutex> lock(mu_);
        read = read_.load(std::memory_order_acquire);
        it = read->m.find(key);
        if (it == read->m.end() && read->amended)
        {
          auto dit = dirty_.find(key);
          if (dit != dirty_.end())
          {
            auto val = dit->second->LoadAndDelete(); // 这里的逻辑需要 Entry 支持 CAS 到 nil
            // C++ 复刻：直接 swap 到 nullptr
            auto old = dit->second->swap_locked(nullptr);
            miss_locked();
            return old;
          }
          miss_locked();
          return nullptr;
        }
      }

      if (it != read->m.end())
      {
        return it->second->swap_locked(nullptr);
      }
      return nullptr;
    }

    void Range(std::function<bool(const K&, const V&)> f)
    {
      auto read = read_.load(std::memory_order_acquire);

      if (read->amended)
      {
        std::lock_guard<std::mutex> lock(mu_);
        read = read_.load(std::memory_order_acquire); // re-load
        if (read->amended)
        {
          // 提升 dirty 到 read
          read_.store(std::make_shared<ReadOnly>(dirty_to_read_locked()), std::memory_order_release);
          dirty_.clear();
          misses_ = 0;
          read = read_.load(std::memory_order_acquire);
        }
      }

      for (const auto& [k, entry] : read->m)
      {
        auto val = entry->load();
        if (val != nullptr)
        {
          if (!f(k, *val))
          {
            break;
          }
        }
      }
    }

  private:
    std::mutex mu_;
    // read_ 必须是 atomic<shared_ptr>，C++20 特性
    std::atomic<std::shared_ptr<ReadOnly>> read_;
    std::unordered_map<K, std::shared_ptr<Entry>> dirty_;
    int misses_ = 0;

    // 当 misses > len(dirty) 时，将 dirty 提升为 read
    void miss_locked()
    {
      misses_++;
      if (misses_ < dirty_.size())
      {
        return;
      }
      // Promotion
      read_.store(std::make_shared<ReadOnly>(dirty_to_read_locked()), std::memory_order_release);
      dirty_.clear();
      misses_ = 0;
    }

    std::unordered_map<K, std::shared_ptr<Entry>> dirty_to_read_locked()
    {
      return dirty_;
    }

    // 当 read.amended 为 false，且有新 key 插入时调用
    // 将 read 中未 expunged 的 entry 复制到 dirty 中
    void dirty_locked()
    {
      if (!dirty_.empty())
        return;

      auto read = read_.load(std::memory_order_acquire);
      for (const auto& [k, entry] : read->m)
      {
        if (!entry->try_expunge_locked())
        {
          // 如果没有被标记为 expunged，说明是有效值或 nil，加入 dirty
          dirty_[k] = entry;
        }
      }
    }
  };
} // namespace cppkit::concurrency