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
        struct Entry;
        using ValuePtr = std::shared_ptr<V>;

        static ValuePtr getExpunged()
        {
            static ValuePtr expunged = std::make_shared<V>();
            return expunged;
        }

        static bool isExpunged(const ValuePtr& p)
        {
            return p == getExpunged();
        }

        struct Entry
        {
            std::shared_ptr<V> p;

            explicit Entry(ValuePtr v) : p(v)
            {
            }

            ValuePtr load() const
            {
                ValuePtr ptr = std::atomic_load(&p);
                if (ptr == nullptr || isExpunged(ptr))
                {
                    return nullptr;
                }
                return ptr;
            }

            bool tryStore(ValuePtr i)
            {
                // 自旋 CAS
                ValuePtr expected = std::atomic_load(&p);
                while (true)
                {
                    if (isExpunged(expected))
                    {
                        return false;
                    }
                    if (std::atomic_compare_exchange_weak(&p, &expected, i))
                    {
                        return true;
                    }
                }
            }

            bool unexpungeLocked()
            {
                ValuePtr expected = getExpunged();
                ValuePtr val = nullptr;
                return std::atomic_compare_exchange_strong(&p, &expected, val);
            }

            void storeLocked(ValuePtr i)
            {
                std::atomic_store(&p, i);
            }

            bool tryExpungeLocked()
            {
                ValuePtr expected = nullptr;
                ValuePtr expunged = getExpunged();
                while (std::atomic_load(&p) == nullptr)
                {
                    if (std::atomic_compare_exchange_weak(&p, &expected, expunged))
                    {
                        return true;
                    }
                    if (expected != nullptr) return false;
                }
                return false;
            }
        };

        struct ReadOnly
        {
            std::unordered_map<K, std::shared_ptr<Entry>> m;
            bool amended = false;
        };

        std::mutex mu;

        std::shared_ptr<ReadOnly> read;

        std::unordered_map<K, std::shared_ptr<Entry>> dirty;
        int misses = 0;

    public:
        SyncMap()
        {
            std::atomic_store(&read, std::make_shared<ReadOnly>());
        }

        // Load
        std::pair<ValuePtr, bool> Load(const K& key)
        {
            // 原子读取 read
            auto r = std::atomic_load_explicit(&read, std::memory_order_acquire);
            auto it = r->m.find(key);

            if (it == r->m.end() && r->amended)
            {
                std::lock_guard lock(mu);
                r = std::atomic_load_explicit(&read, std::memory_order_relaxed);
                it = r->m.find(key);

                if (it == r->m.end() && r->amended)
                {
                    auto dit = dirty.find(key);
                    if (dit != dirty.end())
                    {
                        ValuePtr val = dit->second->load();
                        missLocked();
                        return {val, val != nullptr};
                    }
                    missLocked();
                    return {nullptr, false};
                }
            }

            if (it != r->m.end())
            {
                return {it->second->load(), it->second->load() != nullptr};
            }

            return {nullptr, false};
        }

        // Store
        void Store(const K& key, V value)
        {
            ValuePtr valPtr = std::make_shared<V>(std::move(value));

            auto r = std::atomic_load_explicit(&read, std::memory_order_acquire);
            auto it = r->m.find(key);
            if (it != r->m.end())
            {
                if (it->second->tryStore(valPtr))
                {
                    return;
                }
            }

            std::lock_guard lock(mu);

            r = std::atomic_load_explicit(&read, std::memory_order_relaxed);
            it = r->m.find(key);

            if (it != r->m.end())
            {
                if (it->second->unexpungeLocked())
                {
                    dirty[key] = it->second;
                }
                it->second->storeLocked(valPtr);
            }
            else
            {
                auto dit = dirty.find(key);
                if (dit != dirty.end())
                {
                    dit->second->storeLocked(valPtr);
                }
                else
                {
                    if (!r->amended)
                    {
                        dirtyLocked(r); // Pass r to avoid reload
                        auto nextRead = std::make_shared<ReadOnly>(*r);
                        nextRead->amended = true;
                        std::atomic_store_explicit(&read, nextRead, std::memory_order_release);
                    }
                    dirty[key] = std::make_shared<Entry>(valPtr);
                }
            }
        }

        // Delete
        void Delete(const K& key)
        {
            LoadAndDelete(key);
        }

        // LoadAndDelete
        std::pair<ValuePtr, bool> LoadAndDelete(const K& key)
        {
            auto r = std::atomic_load_explicit(&read, std::memory_order_acquire);
            auto it = r->m.find(key);

            if (it == r->m.end() && r->amended)
            {
                std::lock_guard lock(mu);
                r = std::atomic_load_explicit(&read, std::memory_order_relaxed);
                it = r->m.find(key);

                if (it == r->m.end() && r->amended)
                {
                    auto dit = dirty.find(key);
                    if (dit != dirty.end())
                    {
                        ValuePtr val = dit->second->load();
                        dit->second->storeLocked(nullptr);
                        missLocked();
                        return {val, val != nullptr};
                    }
                    missLocked();
                    return {nullptr, false};
                }
            }

            if (it != r->m.end())
            {
                ValuePtr old = it->second->load();
                it->second->storeLocked(nullptr);
                return {old, old != nullptr};
            }

            return {nullptr, false};
        }

        // Range
        void Range(std::function<bool(const K&, const V&)> f)
        {
            auto r = std::atomic_load_explicit(&read, std::memory_order_acquire);

            if (r->amended)
            {
                std::lock_guard lock(mu);
                r = std::atomic_load_explicit(&read, std::memory_order_relaxed);
                if (r->amended)
                {
                    auto newRead = std::make_shared<ReadOnly>();
                    newRead->m = dirty;
                    std::atomic_store_explicit(&read, newRead, std::memory_order_release);
                    dirty.clear();
                    misses = 0;
                    r = newRead;
                }
            }

            for (const auto& [k, e] : r->m)
            {
                ValuePtr val = e->load();
                if (val != nullptr)
                {
                    if (!f(k, *val))
                    {
                        return;
                    }
                }
            }
        }

    private:
        void missLocked()
        {
            misses++;
            if (misses < dirty.size())
            {
                return;
            }

            auto newRead = std::make_shared<ReadOnly>();
            newRead->m = dirty;
            newRead->amended = false;

            std::atomic_store_explicit(&read, newRead, std::memory_order_release);

            dirty.clear();
            misses = 0;
        }

        void dirtyLocked(const std::shared_ptr<ReadOnly>& r)
        {
            if (!dirty.empty())
            {
                return;
            }

            for (const auto& [k, e] : r->m)
            {
                if (!e->tryExpungeLocked())
                {
                    dirty[k] = e;
                }
            }
        }
    };
} // namespace cppkit::concurrency
