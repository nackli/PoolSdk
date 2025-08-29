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
#pragma once
#ifndef _PLATFORM_PIPE_H_
#define _PLATFORM_PIPE_H_

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif
#include <string>
#include <regex>

#ifdef _WIN32
typedef HANDLE PIPE_HANDLE;
#else
typedef int PIPE_HANDLE;
#endif

enum PIPE_OPT
{
    OPT_READ,
    OPT_WRITE,
    OPT_MAX
};

int createPipe(PIPE_HANDLE hPipe[OPT_MAX]);
PIPE_HANDLE openNamePipe(const char* szFifo);
PIPE_HANDLE createNamePipe(const char* szPipeName);
bool writePipe(PIPE_HANDLE hWrite, const void* data, size_t size);
bool readPipe(PIPE_HANDLE hRead, void* buffer, size_t size);
void closePipe(PIPE_HANDLE hPipe, const char* szPipeName = nullptr);
bool waitConnNamePipe(PIPE_HANDLE hPipe);
#endif