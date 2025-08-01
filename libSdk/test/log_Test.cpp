    
	#include <Windows.h>
	#include "Common/FileLogger.h"
	#pragma comment(lib,"libSdk.lib")
	int main()
	{
		FileLogger::getInstance().initLog("./logCfg.cfg");
		while (1)
		{
			DWORD dwTest = ::GetTickCount();
			for (int i = 1; i < 7000; i++)
			{
				LOG_TRACE("Red-Black Tree after insertion: %i",i);
				LOG_DEBUG("Red-Black Tree after insertion:");
				LOG_INFO("Red-Black Tree after insertion:");
				LOG_WARNING("Red-Black Tree after insertion:");
				LOG_ERROR("Red-Black Tree after insertion:");
				LOG_FATAL("Red-Black Tree after insertion:");
			}

			cout << "diff = " << ::GetTickCount() - dwTest << endl;
		}
	}