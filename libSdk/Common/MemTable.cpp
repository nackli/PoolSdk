/***************************************************************************************************************************************************/
/*
* @Author: Nack Li
* @version 1.0
* @copyright 2025 nackli. All rights reserved.
* @License: MIT (https://opensource.org/licenses/MIT).
* @Date: 2025-08-29
* @LastEditTime: 2025-08-29
*/
/***************************************************************************************************************************************************/
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
    Put(key, ""); 
}

size_t MemTable::ApproximateMemoryUsage() {
    std::lock_guard<std::mutex> lock(m_mtxMemTable);
    return m_szMem;
}

size_t MemTable::getItemCount() {
    std::lock_guard<std::mutex> lock(m_mtxMemTable);
    return m_kvSkipList.size();
}
