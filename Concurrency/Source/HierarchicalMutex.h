#pragma once

#include <mutex>

class HierarchicalMutex
{
public:
    explicit HierarchicalMutex(unsigned long value)
        : m_hierarchyValue(value)
        , m_previousHierarchyValue(0)
    {}

    void lock()
    {
        CheckForHierarchyViolation();
        m_internalMutex.lock();
        UpdateHierarchyValue();
    }

    void unlock()
    {
        m_thisThreadHierarchyValue = m_previousHierarchyValue;
        m_internalMutex.unlock();
    }

    bool try_lock()
    {
        CheckForHierarchyViolation();
        if (!m_internalMutex.try_lock())
            return false;
        UpdateHierarchyValue();
        return true;
    }

private:
    void CheckForHierarchyViolation()
    {
        if (m_thisThreadHierarchyValue <= m_hierarchyValue)
            throw std::logic_error("Mutex hierarchy violated");
    }

    void UpdateHierarchyValue()
    {
        m_previousHierarchyValue = m_thisThreadHierarchyValue;
        m_thisThreadHierarchyValue = m_hierarchyValue;
    }

    std::mutex m_internalMutex;
    const unsigned long m_hierarchyValue;
    unsigned m_previousHierarchyValue;
    static thread_local unsigned long m_thisThreadHierarchyValue;
};
