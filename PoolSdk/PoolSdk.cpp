//
#include <Windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include "thread/AdvancedThreadPool.h"
#include "Common/FileLogger.h"
#include "Common/LockFreeCircularQue.h"
#include "Common/LockQueue.h"
#include "Common/pipe.h"
#include "Common/FileSystem.h"
#include "Common/MemTable.h"
#pragma comment(lib,"libSdk.lib")

#include <winioctl.h>
#include <ntddscsi.h>

#pragma comment(lib, "winmm.lib")
#include <stdio.h>


#include <windows.h>
#include <winioctl.h>
#include <iostream>
#include <vector>

std::wstring GetPhysicalDriveFromLogicalDrive(const wchar_t* logicalDrive) {
    std::wstring result;

    // 获取逻辑磁盘的设备路径
    wchar_t volumeName[MAX_PATH] = { 0 };
    if (!QueryDosDeviceW(logicalDrive, volumeName, MAX_PATH)) {
        std::wcerr << L"QueryDosDevice failed. Error: " << GetLastError() << std::endl;
        return result;
    }

    // 打开卷
    std::wstring volumePath = volumeName;
    HANDLE hVolume = CreateFileW(
        volumePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hVolume == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Failed to open volume. Error: " << GetLastError() << std::endl;
        return result;
    }

    // 获取卷的磁盘扩展信息
    VOLUME_DISK_EXTENTS diskExtents;
    DWORD bytesReturned;

    if (DeviceIoControl(
        hVolume,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        NULL,
        0,
        &diskExtents,
        sizeof(diskExtents),
        &bytesReturned,
        NULL)) {

        if (diskExtents.NumberOfDiskExtents > 0) {
            result = L"\\\\.\\PhysicalDrive" + std::to_wstring(diskExtents.Extents[0].DiskNumber);
        }
    }
    else {
        DWORD err = GetLastError();
        if (err == ERROR_MORE_DATA) {
            // 需要更大的缓冲区
            DWORD size = sizeof(VOLUME_DISK_EXTENTS) +
                (diskExtents.NumberOfDiskExtents - 1) * sizeof(DISK_EXTENT);
            std::vector<BYTE> buffer(size);

            if (DeviceIoControl(
                hVolume,
                IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                NULL,
                0,
                buffer.data(),
                size,
                &bytesReturned,
                NULL)) {

                VOLUME_DISK_EXTENTS* pDiskExtents = reinterpret_cast<VOLUME_DISK_EXTENTS*>(buffer.data());
                if (pDiskExtents->NumberOfDiskExtents > 0) {
                    result = L"\\\\.\\PhysicalDrive" + std::to_wstring(pDiskExtents->Extents[0].DiskNumber);
                }
            }
        }
        else {
            std::wcerr << L"DeviceIoControl failed. Error: " << err << std::endl;
        }
    }

    CloseHandle(hVolume);
    return result;
}

int main()
{
    FileLogger::getInstance().initLog("./logCfg.cfg");
    while (1)
    {
        DWORD dwTest = ::GetTickCount();
        for (int i = 1; i < 30000; i++)
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


    std::wstring physicalDrive = GetPhysicalDriveFromLogicalDrive(L"C:");
    TCHAR szDrive[256] = TEXT("C:\\");
    DWORD dwSerialNumber = 0;
    DWORD dwMaxFileNameLength = 0;
    DWORD dwFileSystemFlags = 0;
    TCHAR szVolumeLabel[MAX_PATH] = TEXT("");
    TCHAR szFileSystemName[MAX_PATH] = TEXT("");

    if (GetVolumeInformation(
        szDrive,         // 驱动器号，如 "C:\\"
        szVolumeLabel,   // 卷标缓冲区
        MAX_PATH,        // 卷标缓冲区的大小
        &dwSerialNumber, // 序列号缓冲区
        &dwMaxFileNameLength, // 最大文件名长度
        &dwFileSystemFlags, // 文件系统标志
        szFileSystemName,  // 文件系统名称缓冲区
        MAX_PATH)) {      // 文件系统名称缓冲区的大小
        std::wcout << L"Drive Label: " << szVolumeLabel << std::endl;
        std::wcout << L"File System: " << szFileSystemName << std::endl;
        std::wcout << L"Serial Number: " << dwSerialNumber << std::endl;
    }
    else {
        std::wcerr << L"Failed to get volume information." << std::endl;
    }


    string str = NtPathToDosPath("f:\\");
    LARGE_INTEGER start, end, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    // Perform I/O operation here (e.g., read/write file)
    QueryPerformanceCounter(&end);
    double duration = double(end.QuadPart - start.QuadPart) / double(frequency.QuadPart);
    std::cout << "I/O operation took " << duration << " seconds." << std::endl;

    void *ptr = new int;

    AdvancedThreadPool pool(2, 4);
    PIPE_HANDLE hPipe[OPT_MAX];

    LOG_DEBUG("High priority task running %d writePipe=%s");
    //auto p1 = new int64_t;
    //auto p2 = new int64_t[12];

    hPipe[OPT_WRITE] = createNamePipe("\\\\.\\pipe\\Test");

    hPipe[OPT_READ] = createNamePipe("\\\\.\\pipe\\Test");
    auto high_priority_task = pool.enqueue(
        AdvancedThreadPool::Priority::Normal,
        [&pool, hPipe]() {
            while (1)
            {
                auto threadId = std::this_thread::get_id();
                const char* szTestPipe = "hello nack";
                //writePipe(hPipe[OPT_WRITE], szTestPipe,strlen(szTestPipe));
                LOG_TRACE("High priority task running %d writePipe=%s", threadId, szTestPipe);
                LOG_DEBUG("High priority task running %d writePipe=%s", threadId, szTestPipe);
                LOG_INFO("High priority task running %d writePipe=%s", threadId, szTestPipe);
                LOG_WARNING("High priority task running %d writePipe=%s", threadId, szTestPipe);
                LOG_ERROR("High priority task running %d writePipe=%s", threadId, szTestPipe);
                LOG_FATAL("High priority task running %d writePipe=%s", threadId, szTestPipe);
                //std::cout << " High priority task running " << threadId << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));;
            }
     

        }
    );

    auto high_priority_taskT = pool.enqueue(
        AdvancedThreadPool::Priority::Normal,
        [&pool, hPipe]() {
            while (1)
            {
                auto threadId = std::this_thread::get_id();
                char szData[256] = { 0 };
                //readPipe(hPipe[OPT_READ], szData, sizeof(szData));
                LOG_DEBUG("High priority task running %d readPipe=%s", threadId, szData);
                //std::cout << " High priority task running " << threadId << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));;
            }
        }
    );

    system("pause");
    std::cout << "Hello World!\n";
}
