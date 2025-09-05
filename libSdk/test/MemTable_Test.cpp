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
#ifdef _WIN32
#include <Windows.h>
#endif
#include "Common/MemTable.h"
int main()
{
    MemTable memtable;

    memtable.Put("key1", "value1");
    memtable.Put("key2", "value2");
    memtable.Put("key3", "value3");
	
    std::string value;
    if (memtable.Get("key2", value)) {
        std::cout << "Found key2: " << value << std::endl;
    }
    memtable.Put("key2", "value212314124");
    if (memtable.Get("key2", value)) {
        std::cout << "1111 Found key2: " << value << std::endl;
    }
    memtable.Delete("key2");	

    std::cout << "MemTable size: " << memtable.getItemCount() << std::endl;
    std::cout << "Memory usage: " << memtable.ApproximateMemoryUsage() << " bytes" << std::endl;	
}
