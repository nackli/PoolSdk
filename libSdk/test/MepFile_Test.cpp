/*
 * @Author: nackli nackli@163.com
 * @Date: 2025-08-01 21:20:49
 * @LastEditors: nackli nackli@163.com
 * @LastEditTime: 2025-08-01 22:04:35
 * @FilePath: /PoolSdk/PoolSdk/PoolSdk.cpp
 * */
//
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <vector>
#include <list>
#include "FileLogger/FileLogger.h"
#include "Common/StringUtils.h"
#include "Common/MapFile.h"
#ifdef _WIN32
#pragma comment(lib,"libSdk.lib")
#endif

HANDLE g_hNotifyEvent = nullptr;
static void OnWriteData(MapFile* pData)
{
    while (1)
    {
#if 1
        static int iInde = 0;
        char szBuf[2048];
        sprintf(szBuf, "{\"SVACEncodeConfig\":{\"nihq291234haf\":{\"ROIFlag\":%d,\"ROINumber\":3,\"Item\":[{\"ROISeq\":1,\"TopLeft\":0,\"BottomRight\":0,\"ROIQP\":0},{\"ROISeq\":2,\"TopLeft\":0,\"BottomRight\":0,\"ROIQP\":1},{\"ROISeq\":3,\"TopLeft\":0,\"BottomRight\":0,\"ROIQP\":2}]},\"SVCParam\":{\"SVCSTMMode\":0,\"SVCSpaceDomainMode\":0,\"SVCTimeDomainMode\":0,\"SSVCRatioValue\":\"4:1\",\"SVCSpaceSupportMode\":0,\"SVCTimeSupportMode\":0,\"SSVCRatioSupportList\":\"4:3/2:1/4:1/6:1/8:1\"},\"SurveillanceParam\":{\"TimeFlag\":0,\"OSDFlag\":0,\"AIFlag\":0,\"GISFlag\":0},\"AudioParam\":{\"AudioRecognitionFlag\":%d}}}\n", rand(), iInde);
        pData->writeMap(szBuf);
        iInde++;

        sprintf(szBuf, "{\"SVACEncodeConfig\":{\"ROIParam\":{\"ROIFlag\":%d,\"ROINumber\":3,\"Item\":[{\"ROISeq\":1,\"TopLeft\":0,\"BottomRight\":0,\"ROIQP\":0},{\"ROISeq\":2,\"TopLeft\":0,\"BottomRight\":0,\"ROIQP\":1},{\"ROISeq\":3,\"TopLeft\":0,\"BottomRight\":0,\"ROIQP\":2}]},\"SVCParam\":{\"SVCSTMMode\":0,\"SVCSpaceDomainMode\":0,\"SVCTimeDomainMode\":0,\"SSVCRatioValue\":\"4:1\",\"SVCSpaceSupportMode\":0,\"SVCTimeSupportMode\":0,\"SSVCRatioSupportList\":\"4:3/2:1/4:1/6:1/8:1\"},\"SurveillanceParam\":{\"TimeFlag\":0,\"OSDFlag\":0,\"AIFlag\":0,\"GISFlag\":0},\"AudioParam\":{\"AudioRecognitionFlag\":%d}}}\n", rand(), iInde);
        pData->writeMap(szBuf);
        iInde++;

        sprintf(szBuf, "{\"SVACEncodeConfig\":{\"china zhongguo\":{\"ROIFlag\":%d,\"ROINumber\":3,\"Item\":[{\"ROISeq\":1,\"TopLeft\":0,\"BottomRight\":0,\"ROIQP\":0},{\"ROISeq\":2,\"TopLeft\":0,\"BottomRight\":0,\"ROIQP\":1},{\"ROISeq\":3,\"TopLeft\":0,\"BottomRight\":0,\"ROIQP\":2}]},\"SVCParam\":{\"SVCSTMMode\":0,\"SVCSpaceDomainMode\":0,\"SVCTimeDomainMode\":0,\"SSVCRatioValue\":\"4:1\",\"SVCSpaceSupportMode\":0,\"SVCTimeSupportMode\":0,\"SSVCRatioSupportList\":\"4:3/2:1/4:1/6:1/8:1\"},\"SurveillanceParam\":{\"TimeFlag\":0,\"OSDFlag\":0,\"AIFlag\":0,\"GISFlag\":0},\"AudioParam\":{\"AudioRecognitionFlag\":%d}}}\n", rand(), iInde);
        pData->writeMap(szBuf);
        iInde++;
        Sleep(10);
        ::SetEvent(g_hNotifyEvent);
#else
        string strData = randomJsonString(50,200);
        if (strData[strData.length() - 1] != '\n')
            strData += '\n';
		if (pData->writeMap(strData.c_str()))
			::SetEvent(g_hNotifyEvent);
		Sleep(1);
#endif

    }

}

static void OnReadData(MapFile* pData)
{
    while (1)
    {
		WaitForSingleObject(g_hNotifyEvent, - 1);
        bool bDeleteMem = false;
        auto mapData = pData->readMap(bDeleteMem);
		if (mapData.first && mapData.second)
		{
			pData->moveReadOffset(mapData.second);
		}
		else
			ResetEvent(g_hNotifyEvent);
    }
}

int main()
{
	g_hNotifyEvent = CreateEvent(nullptr, false, false, nullptr);

    MapFile mapFile;
    mapFile.openOrCreateMap("Test", 0x6400000);

    for (int i = 0; i < 1; i++)
    {
        std::thread tWrite(OnWriteData, &mapFile);
        tWrite.detach();
    }

    std::thread tRead(OnReadData, &mapFile);
    tRead.detach();
    while (1)
#ifdef _WIN32
        ::Sleep(10000000);
#else
        sleep(10000000);
#endif

    std::cout << "Hello World!\n";
}
