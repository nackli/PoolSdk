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
#pragma once
#include "SkipListKv.h"
class MemTable {
public:
    // add kv
    void Put(const std::string& key, const std::string& value);

    //get key value
    bool Get(const std::string& key, std::string& value);

    // delete key
    void Delete(const std::string& key);
    // get memory usage
    size_t ApproximateMemoryUsage();

    // get item num
    size_t getItemCount();

private:
    SkipListKv<std::string, std::string> m_kvSkipList;
    std::mutex m_mtxMemTable;
    size_t m_szMem = 0;
};
