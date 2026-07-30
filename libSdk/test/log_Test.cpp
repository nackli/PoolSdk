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
#include "./FileLogger/FileLogger.h"
#include "Common/LockQueue.h"
#ifdef _WIN32
#include <Windows.h>
#else
unsigned long GetTickCount()
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif

enum test_level
{
	test_level_0,
	test_level_1,
	test_level_2,
	test_level_3,
	test_level_4,
	test_level_5,
	test_level_6,
	test_level_7,
	test_level_8,
	test_level_9,
	test_level_10,
	test_level_11,
	test_level_12,
	test_level_13,
	test_level_14,
	test_level_15,
	test_level_16,
	test_level_17,
	test_level_18,
	test_level_19,
	test_level_20,
	test_level_21,
	test_level_22,
	test_level_23,
	test_level_24,
	test_level_25,
	test_level_26,
	test_level_27,
	test_level_28,
	test_level_29,
	test_level_max
};

// 定义 format_as，返回字符串视图

constexpr std::string_view format_as(test_level level) {
    switch (level) {
        case test_level::test_level_22: return "Level1";
        case test_level::test_level_29: return "Level2";
        // 添加所有枚举值
        default: return "Unknown";
    }
}

int main()
{
	FileLogger::getInstance().initLog("./logCfg.json");
	//FileLogger::getInstance().setLogFileName()
	const int iCntNum = 45000;
	while (1)
	{
		unsigned long dwTest = ::GetTickCount();
		for (int i = 0; i < iCntNum; i++)
		{
#if 0
			LOG_TRACE("Welcome to nack log space: %c", i);
			LOG_DEBUG("Welcome to nack log space:");
			LOG_INFO("Welcome to nack log space::");
			LOG_WARN("Welcome to nack log space::");
			LOG_ERROR("Welcome to nack log space::");
			LOG_FATAL("Welcome to nack log space::");
#else
			LOG_TRACE_S("Welcome to nack log space:: {}", i);
			LOG_DEBUG_S("Welcome to nack log space:: {}",test_level_27);
			LOG_INFO_S("Welcome to nack log space::");
			LOG_WARN_S("Welcome to nack log space::");
			LOG_ERROR_S("Welcome to nack log space::");
			LOG_FATAL_S("Welcome to nack log space::");
			LOG_ERROR_S("Create file mapping fail: {}", 6851);
#endif
		}

		std::cout << "diff = " << " " << ::GetTickCount() - dwTest << std::endl;
	}
}