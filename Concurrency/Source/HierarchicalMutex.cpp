#include "HierarchicalMutex.h"

thread_local unsigned long HierarchicalMutex::m_thisThreadHierarchyValue{ ULONG_MAX };