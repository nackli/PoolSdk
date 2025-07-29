//
#include <Windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include "thread/AdvancedThreadPool.h"
#include "mem/Pool_Test.h"
#include "Common/FileLogger.h"
#include "Common/LockFreeCircularQue.h"
#include "Common/CircularQueue.h"
#include "Common/pipe.h"
#include "Common/FileSystem.h"
#include "Common/MemTable.h"
#pragma comment(lib,"libSdk.lib")

#include <winioctl.h>
#include <ntddscsi.h>

#pragma comment(lib, "winmm.lib")
#include <stdio.h>


void GetDiskPerformance(const std::string& drive_path) {
    HANDLE hDisk = CreateFileA(drive_path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDisk == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open disk " << drive_path << ". Error code: " << GetLastError() << std::endl;
        return;
    }

    DISK_PERFORMANCE disk_performance = {};
    DWORD bytes_returned = 0;
    if (!DeviceIoControl(hDisk, IOCTL_DISK_PERFORMANCE, nullptr, 0, &disk_performance, sizeof(disk_performance), &bytes_returned, nullptr)) {
        std::cerr << "Failed to get performance data for " << drive_path << ". Error code: " << GetLastError() << std::endl;
        CloseHandle(hDisk);
        return;
    }

    std::cout << "Drive " << drive_path << " performance:" << std::endl;
    std::cout << "    BytesRead: " << disk_performance.BytesRead.QuadPart << std::endl;
    std::cout << "    BytesWritten: " << disk_performance.BytesWritten.QuadPart << std::endl;
    std::cout << "    ReadCount: " << disk_performance.ReadCount << std::endl;
    std::cout << "    WriteCount: " << disk_performance.WriteCount << std::endl;
    std::cout << "    ReadTime: " << disk_performance.ReadTime.QuadPart << " ns" << std::endl;
    std::cout << "    WriteTime: " << disk_performance.WriteTime.QuadPart << " ns" << std::endl;
    std::cout << "    IdleTime: " << disk_performance.IdleTime.QuadPart << " ns" << std::endl;
    std::cout << "    ReadBytesPerSec: " << disk_performance.BytesRead.QuadPart * 10000000.0 / disk_performance.ReadTime.QuadPart << " B/s" << std::endl;
    std::cout << "    WriteBytesPerSec: " << disk_performance.BytesWritten.QuadPart * 10000000.0 / disk_performance.WriteTime.QuadPart << " B/s" << std::endl;
    std::cout << "    ReadCountPerSec: " << disk_performance.ReadCount * 10000000.0 / disk_performance.ReadTime.QuadPart << " IO/s" << std::endl;
    std::cout << "    WriteCountPerSec: " << disk_performance.WriteCount * 10000000.0 / disk_performance.WriteTime.QuadPart << " IO/s" << std::endl;

    /* STORAGE_PROPERTY_QUERY query = {};
     query.PropertyId = StorageDeviceProperty;
     query.QueryType = PropertyStandardQuery;

     STORAGE_DESCRIPTOR_HEADER device_descriptor = {};
     if (!DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &device_descriptor, sizeof(device_descriptor), &bytes_returned, nullptr)) {
         std::cerr << "Failed to get device descriptor for " << drive_path << ". Error code: " << GetLastError() << std::endl;
         CloseHandle(hDisk);
         return;
     }


     if (device_descriptor.BusType == BusTypeSata) {
         ATA_PASS_THROUGH_DIRECT ata_command = {};
         ata_command.Length = sizeof(ATA_PASS_THROUGH_DIRECT);
         ata_command.AtaFlags = ATA_FLAGS_DATA_IN;
         ata_command.DataTransferLength = sizeof(IDENTIFY_DEVICE_DATA);
         ata_command.TimeOutValue = 10;
         ata_command.DataBuffer = new UCHAR[sizeof(IDENTIFY_DEVICE_DATA)];
         memset(ata_command.DataBuffer, 0, sizeof(IDENTIFY_DEVICE_DATA));

         ATA_IDENTIFY_DEVICE& identify_device = *(ATA_IDENTIFY_DEVICE*)ata_command.DataBuffer;
         identify_device.CommandCode = ATA_IDENTIFY_DEVICE;
         if (!DeviceIoControl(hDisk, IOCTL_ATA_PASS_THROUGH_DIRECT, &ata_command, sizeof(ata_command), &ata_command, sizeof(ata_command), &bytes_returned, nullptr) ||
             bytes_returned != sizeof(ata_command)) {
             std::cerr << "Failed to get ATA IDENTIFY DEVICE data for " << drive_path << ". Error code: " << GetLastError() << std::endl;
             delete[] ata_command.DataBuffer;
             CloseHandle(hDisk);
             return;
         }

         IDENTIFY_DEVICE_DATA& identify_data = *(IDENTIFY_DEVICE_DATA*)ata_command.DataBuffer;
         char model_number[41] = {};
         memcpy(model_number, identify_data.ModelNumber, sizeof(identify_data.ModelNumber));
         std::cout << "    ModelNumber: " << model_number << std::endl;

         delete[] ata_command.DataBuffer;*/
         //}
         //else {
         //    STORAGE_PROPERTY_QUERY query = {};
         //    query.PropertyId = StorageDeviceProperty;
         //    query.QueryType = PropertyStandardQuery;

         //    STORAGE_DEVICE_DESCRIPTOR device_descriptor = {};
         //    if (!DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &device_descriptor, sizeof(device_descriptor), &bytes_returned, nullptr)) {
         //        std::cerr << "Failed to get device descriptor for " << drive_path << ". Error code: " << GetLastError() << std::endl;
         //        CloseHandle(hDisk);
         //        return;
         //    }

         //    char vendor_id[9] = {};
         //    memcpy(vendor_id, device_descriptor.VendorIdOffset + (char*)&device_descriptor, device_descriptor.VendorIdOffset ? 8 : 0);
         //    std::cout << "    VendorId: " << vendor_id << std::endl;

         //    char product_id[17] = {};
         //    memcpy(product_id, device_descriptor.ProductIdOffset + (char*)&device_descriptor, device_descriptor.ProductIdOffset ? 16 : 0);
         //    std::cout << "    ProductId: " << product_id << std::endl;
         //}

    CloseHandle(hDisk);
}
//void* operator new(std::size_t szMem) {
//    if (szMem == 0)
//        szMem = 1;
//    void* ptrData = ConcurrentAllocate(szMem);
//    if (void* ptr = ConcurrentAllocate(szMem))
//        return ptr;
//    throw std::bad_alloc{};
//    return nullptr;
//}
//
//void* operator new[](size_t size) noexcept(false) {
//    std::printf("call:%s %zu\n", __func__, size);
//
//    if (size == 0) {
//        size = 1;
//    }
//    if (void* ptr = std::malloc(size)) {
//        return ptr;
//    }
//    throw std::bad_alloc{};
//}

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


#include <windows.h>
#include <iostream>
#include "Common/SkipList.h"
#include "Common/RBTree.h"
#include "Common/hashAlg.h"

int main()
{
    //for (int i = 1; i < 0xff; i++)
    //{
    //    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), i);
    //    printf("%02d", i);
    //    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY);//设置颜色，没有添加颜色，故为原色
    //    printf(" ");
    //}
    printf("\n");
    FileLogger::getInstance().initLog("./logCfg.cfg");
    LOG_TRACE("Red-Black Tree after insertion:");
    LOG_DEBUG("Red-Black Tree after insertion:");
    LOG_INFO("Red-Black Tree after insertion:");
    LOG_WARNING("Red-Black Tree after insertion:");
    LOG_ERROR("Red-Black Tree after insertion:");
    LOG_FATAL("Red-Black Tree after insertion:");

    uint32_t uHash = ELFhash("abfadfaeirtwejginfiqhvbcvmfkr hvqa");
    RedBlackTree<int> rbTree;

    rbTree.insert(7);
    rbTree.insert(3);
    rbTree.insert(18);
    rbTree.insert(10);
    rbTree.insert(22);
    rbTree.insert(8);
    rbTree.insert(11);
    rbTree.insert(26);

    cout << "Red-Black Tree after insertion:" << endl;
    rbTree.printTree();

    cout << "\nDeleting 18..." << endl;
    rbTree.remove(18);
    rbTree.printTree();

    cout << "\nSearching for 11: ";
    auto result = rbTree.search(11);
    if (result) {
        cout << "Found (" << result->data << ")" << endl;
    }
    else {
        cout << "Not found" << endl;
    }

    MemTable memtable;

    memtable.Put("key1", "value1");
    memtable.Put("key2", "value2");
    memtable.Put("key3", "value3");
    SkipList<int> skipList;
    for (int i = 0; i < 300; i++)
        skipList.insert(20 + i);

    bool b = skipList.find(25);
    bool c = skipList.find(35);

    skipList.printHelper();

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


    ConcurrentAllocate(100);
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
                LOG_DEBUG("High priority task running %d writePipe=%s", threadId, szTestPipe);
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

    while (1)
    {
        size_t n = 1000;
        cout << "==========================================================" << endl;
        //std::vector<void*> v;
        //for (size_t i = 0; i < n; i++)
        //{
        //    //v.push_back(ConcurrentAlloc(16));
        //    v.push_back(ConcurrentAllocate((16 + i) % 8192 + 1));
        //}
        //size_t end1 = clock();

        //size_t begin2 = clock();
        //for (size_t i = 0; i < n; i++)
        //{
        //    ConcurrentFree(v[i]);
        //}
        //cout << endl << endl;

        BenchmarkConcurrentMalloc(n, 100, 100);
        //BenchmarkMalloc(n, 100, 100);
        //cout << "==========================================================" << endl;
    }

 //   BenchmarkConcurrentMalloc(1, 1, 1);
    system("pause");
    std::cout << "Hello World!\n";
}
