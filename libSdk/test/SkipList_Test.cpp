    
#include <Windows.h>
#include "Common/SkipList.h"
#pragma comment(lib,"libSdk.lib")
int main()
{

    SkipList<int> skipList;
    for (int i = 0; i < 300; i++)
        skipList.insert(20 + i);

    bool b = skipList.find(25);
    bool c = skipList.find(35);

    skipList.printHelper();
	
	
	
	
}
