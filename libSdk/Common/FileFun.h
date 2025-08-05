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
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif
#define ACCESS_READ                     0x0000
#define ACCESS_WRITE                    0x0001
#define ACCESS_RDWR                     0x0002

#define ACCESS_APPEND                   0x0008
#define ACCESS_CREAT                    0x0100  // create and open file
#define ACCESS_TRUNC                    0x0200  // open and truncate
#define ACCESS_EXCL                     0x0400  // open only if file doesn't already exist
#define ACCESS_SHART                    0x1000  // share mode

FHANDLE openDevice(const char* szName, int iFlags);
void closeDevice(FHANDLE& hFile);

bool writeDevice(FHANDLE hWrite, const void* data, size_t size);

bool readDevice(FHANDLE hRead, void* buffer, size_t size);


bool seekDevice(FHANDLE hFile, size_t iPos, int iMoveMethod); 
bool delDevice(const char* szFileName);
bool snapshotLink(const char* szOldFileName, const char* szNewFileName);
int getFileSize(const char* szFileName);

#endif
