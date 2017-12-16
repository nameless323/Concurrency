#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <list>
#include <map>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ThreadSafeLookupTable
{
public:
    ThreadSafeLookupTable(unsigned int numBuckets = 19, const Hash& hasher = Hash())
        : m_buckets(numBuckets), m_hasher(hasher)
    {
        for (unsigned int i = 0; i < numBuckets; ++i)
        {
            m_buckets[i].reset(new BucketType);
        }
    }

    ThreadSafeLookupTable(const ThreadSafeLookupTable& other) = delete;
    ThreadSafeLookupTable& operator=(const ThreadSafeLookupTable& other) = delete;

    Value ValueFor(const Key& key, const Value& defaultValue = Value()) const
    {
        return GetBucket(key).ValueFor(key, defaultValue);
    }

    void AddOrUpdateMapping(const Key& key, const Value& value)
    {
        GetBucket(key).AddOrUpdateMapping(key, value);
    }

    void RemoveMapping(const Key& key)
    {
        GetBucket(key).RemoveMapping(key);
    }

    std::map<Key, Value> GetMap() const
    {
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for (unsigned int i = 0; i < m_buckets.size(); ++i)
            locks.push_back(std::unique_lock<std::shared_mutex>(m_buckets[i]->m_mutex));
        std::map<Key, Value> res;
        for (unsigned int i = 0; i < m_buckets.size(); ++i)
        {
            for (auto it = m_buckets[i].m_data.begin(); it != m_buckets[i].m_data.end(); ++i)
                res.insert(*it);
        }
        return res;
    }

private:
    class BucketType
    {
    public:
        Value ValueFor(const Key& key, const Value& defaultValue) const
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            const BucketIterator fountEntry = FindEntryFor(key);
            return fountEntry == m_data.end() ? defaultValue : fountEntry->second;
        }

        void AddOrUpdateMapping(const Key& key, const Value& value)
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            const BucketIterator foundEntry = FindEntryFor(key);
            if (foundEntry == m_data.end())
                m_data.push_back(BucketValue(key, value));
            else
                foundEntry->second = value;
        }

        void RemoveMapping(const Key& key)
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            const BucketIterator foundEntry = FindEntryFor(key);
            if (foundEntry != m_data.end())
                m_data.erase(foundEntry);
        }

    private:
        using BucketValue = std::pair<Key, Value>;
        using BucketData = std::list<BucketValue>;
        using BucketIterator = typename BucketData::iterator;

        BucketData m_data;
        mutable std::shared_mutex m_mutex;

        BucketIterator FindEntryFor(const Key& key) const
        {
            return std::find_if(m_data.begin(), m_data.end(), 
                [&key](const BucketValue& i)
                {
                    return i.first == key;
                });
        }
    };

    std::vector<std::unique_ptr<BucketType>> m_buckets;
    Hash m_hasher;

    BucketType& GetBucket(const Key& key) const
    {
        const std::size_t bucketIndex = m_hasher(key) % m_buckets.size();
        return *m_buckets[bucketIndex];
    }
};