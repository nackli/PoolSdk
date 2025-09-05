    
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
#ifdef _DEBUG
#pragma comment(lib,"PoolSdk_d.lib")
#else
#pragma comment(lib,"PoolSdk.lib")
#endif
#endif
#include "Common/SkipList.h"
int main()
{

    SkipList<int> skipList;
    for (int i = 0; i < 300; i++)
        skipList.insert(20 + i);

    bool b = skipList.find(25);
    bool c = skipList.find(35);

    skipList.printHelper();
	
	
	
	
}
