    
	#include <Windows.h>
	#include "Common/FileLogger.h"
	#pragma comment(lib,"libSdk.lib")
	int main()
	{
		FileLogger::getInstance().initLog("./logCfg.cfg");
		const int iCntNum = 45000;
		while (1)
		{
			unsigned long dwTest = ::GetTickCount();
			for (int i = 0; i < iCntNum; i++)
			{
	#if 0
				LOG_TRACE("Welcome to nack log space: %d", i);
				LOG_DEBUG("Welcome to nack log space:");
				LOG_INFO("Welcome to nack log space::");
				LOG_WARN("Welcome to nack log space::");
				LOG_ERROR("Welcome to nack log space::");
				LOG_FATAL("Welcome to nack log space::");
	#else
				LOG_TRACE_S("Welcome to nack log space:: {}", i);
				LOG_DEBUG_S("Welcome to nack log space::");
				LOG_INFO_S("Welcome to nack log space::");
				LOG_WARN_S("Welcome to nack log space::");
				LOG_ERROR_S("Welcome to nack log space::");
				LOG_FATAL_S("Welcome to nack log space::");
	#endif
			}

			cout << "diff = " << " " << ::GetTickCount() - dwTest << endl;
		}
	}