#pragma once
#ifndef _PLATFORM_FILE_FUN_H_
#define _PLATFORM_FILE_FUN_H_
#ifdef _WIN32
#include <Windows.h>
typedef HANDLE FHANDLE;
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
typedef int FHANDLE;
#endif
#include <stdexcept>
#define ACCESS_READ                     0x0000
#define ACCESS_WRITE                    0x0001
#define ACCESS_RDWR                     0x0002

#define ACCESS_APPEND                   0x0008
#define ACCESS_CREAT                    0x0100  // create and open file
#define ACCESS_TRUNC                    0x0200  // open and truncate
#define ACCESS_EXCL                     0x0400  // open only if file doesn't already exist
#define ACCESS_SHART                    0x1000  // share mode

FHANDLE openDevice(const char* szName, int iFlags)
{
#ifdef _WIN32
    DWORD dwFlag = GENERIC_READ;
    DWORD dwMode = OPEN_ALWAYS;
    if ((iFlags & ACCESS_WRITE) == ACCESS_WRITE)
        dwFlag = GENERIC_WRITE;
    else if ((iFlags & ACCESS_RDWR) == ACCESS_RDWR)
        dwFlag = GENERIC_READ | GENERIC_WRITE;

    DWORD dwShare = 0;

    if (iFlags & ACCESS_SHART)
    {
        if ((iFlags & ACCESS_WRITE) == ACCESS_WRITE)
            dwShare |= FILE_SHARE_WRITE;
        else if ((iFlags & ACCESS_READ) == ACCESS_READ)
            dwShare |= FILE_SHARE_READ;
    }

    if ((iFlags & ACCESS_CREAT) == ACCESS_CREAT && (iFlags & ACCESS_EXCL) == ACCESS_EXCL)
        dwMode = CREATE_NEW;
    else if ((iFlags & ACCESS_CREAT) == ACCESS_CREAT)
        dwMode = CREATE_ALWAYS;

    if ((iFlags & ACCESS_TRUNC) == ACCESS_TRUNC)
        dwMode = TRUNCATE_EXISTING;

    if ((iFlags & ACCESS_APPEND) == ACCESS_APPEND)
        dwMode = OPEN_EXISTING;


    FHANDLE hFile = CreateFileA(szName,
        dwFlag,
        0,
        NULL,
        dwMode,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
        throw std::runtime_error("Failed to open device");
#else
    FHANDLE hFile = open(devicePath.c_str(), O_RDWR);
    if (hFile == -1) 
        throw std::runtime_error("Failed to open device");
#endif
    return hFile;
}

void closeDevice(FHANDLE hFile)
{
#ifdef _WIN32
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
#else
    if (hFile != -1) {
        close(hFile);
    }
#endif
}

// 写入数据
bool writeDevice(FHANDLE hWrite, const void* data, size_t size) {
#ifdef _WIN32
    DWORD written;
    return WriteFile(hWrite, data, (DWORD)size, &written, NULL);
#else
    return ::write(hWrite, data, size) != -1;
#endif
}

// 读取数据
bool readDevice(FHANDLE hRead, void* buffer, size_t size) {
#ifdef _WIN32
    DWORD read;
    return ReadFile(hRead, buffer, (DWORD)size, &read, NULL);
#else
    return ::read(hRead, buffer, size) != -1;
#endif
}

// 读取数据
bool seekDevice(FHANDLE hRead, size_t iPos, int iMoveMethod) {
#ifdef _WIN32
    return SetFilePointer(hRead, (long)iPos, nullptr, (DWORD)iMoveMethod) != INVALID_SET_FILE_POINTER;
#else
    return lseek(hRead, buffer, size) != -1;
#endif
}

bool delDevice(const char* szFileName)
{
    if (!szFileName)
        return false;
    return remove(szFileName) == 0;
}


bool snapshotLink(const char* szOldFileName,const char *szNewFileName)
{
    if (!szOldFileName || !szNewFileName)
        return false;
    return true;
    //return readlink(szOldFileName, szNewFileName) == 0;
}

int getFileSize(const char* szFileName)
{
#ifdef _WIN32
    HANDLE fileHandle = openDevice(szFileName, ACCESS_READ | ACCESS_SHART);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        return -1;  // 返回-1表示获取文件大小失败
    }
    int fileSize = GetFileSize(fileHandle, NULL);
    closeDevice(fileHandle);
    return fileSize;
#else
    struct stat st;
    if (stat(fileName, &st) == -1) {
        return -1;  // 返回-1表示获取文件大小失败
    }
    return st.st_size;
#endif
}

#endif
