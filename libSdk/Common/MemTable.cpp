#include "MemTable.h"

void MemTable::Put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mtxMemTable);

    // calc mem change
    std::string old_value;
    if (m_kvSkipList.find(key, old_value)) {
        m_szMem -= old_value.size();
    }

    m_kvSkipList.insert(key, value);
    m_szMem += key.size() + value.size();
}

bool MemTable::Get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(m_mtxMemTable);
    return m_kvSkipList.find(key, value);
}

void MemTable::Delete(const std::string& key) {
    Put(key, "");  // 使用空字符串作为墓碑标记
}

size_t MemTable::ApproximateMemoryUsage() {
    std::lock_guard<std::mutex> lock(m_mtxMemTable);
    return m_szMem;
}

size_t MemTable::getItemCount() {
    std::lock_guard<std::mutex> lock(m_mtxMemTable);
    return m_kvSkipList.size();
}
