#include "FileFun.h"
#include <unistd.h>

FHANDLE openDevice(const char* szName, int iFlags)
{
#ifdef _WIN32 
    if (!szName)
        return nullptr;

    DWORD dwFlag = GENERIC_READ;
    DWORD dwMode = OPEN_ALWAYS;
    if ((iFlags & ACCESS_WRITE) == ACCESS_WRITE)
        dwFlag = GENERIC_WRITE;
    else if ((iFlags & ACCESS_RDWR) == ACCESS_RDWR)
        dwFlag = GENERIC_READ | GENERIC_WRITE;

    DWORD dwShare = 0;

    if (iFlags & ACCESS_SHART)
    {
        if ((iFlags & ACCESS_RDWR) == ACCESS_RDWR)
            dwShare |= (FILE_SHARE_READ | FILE_SHARE_WRITE);
        if ((iFlags & ACCESS_WRITE) == ACCESS_WRITE)
            dwShare |= FILE_SHARE_WRITE;
        if ((iFlags & ACCESS_READ) == ACCESS_READ)
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
    if (!szName)
        return -1;
        
    FHANDLE hFile = open(szName, O_RDWR);
    if (hFile == -1)
        throw std::runtime_error("Failed to open device");
#endif
    return hFile;
}

void closeDevice(FHANDLE &hFile)
{
    if (hFile != INVALID_HANDLE_VALUE) {
#ifdef _WIN32
        FlushFileBuffers(hFile);
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
#else
        close(hFile);
#endif
    }

}

// д������
bool writeDevice(FHANDLE hWrite, const void* data, size_t size) {
    if (!data || hWrite == INVALID_HANDLE_VALUE)
        return false;
#ifdef _WIN32
    DWORD written;
    return WriteFile(hWrite, data, (DWORD)size, &written, NULL);
#else
    return ::write(hWrite, data, size) != -1;
#endif
}

// ��ȡ����
bool readDevice(FHANDLE hRead, void* buffer, size_t size) {
    if (!buffer || hRead == INVALID_HANDLE_VALUE)
        return false;
#ifdef _WIN32
    DWORD read;
    return ReadFile(hRead, buffer, (DWORD)size, &read, NULL);
#else
    return ::read(hRead, buffer, size) != -1;
#endif
}

// ��ȡ����
bool seekDevice(FHANDLE hFile, size_t iPos, int iMoveMethod) {
    if (hFile == INVALID_HANDLE_VALUE)
        return false;
#ifdef _WIN32
    return SetFilePointer(hFile, (long)iPos, nullptr, (DWORD)iMoveMethod) != INVALID_SET_FILE_POINTER;
#else
    return lseek(hFile, iPos, iMoveMethod) != -1;
#endif
}

bool delDevice(const char* szFileName)
{
    if (!szFileName)
        return false;
    return remove(szFileName) == 0;
}


bool snapshotLink(const char* szOldFileName, const char* szNewFileName)
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
        return -1;  // ����-1��ʾ��ȡ�ļ���Сʧ��
    }
    int fileSize = GetFileSize(fileHandle, NULL);
    closeDevice(fileHandle);
    return fileSize;
#else
    struct stat st;
    if (stat(szFileName, &st) == -1) {
        return -1;  // ����-1��ʾ��ȡ�ļ���Сʧ��
    }
    return st.st_size;
#endif
}


