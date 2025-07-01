/*
Written by Nack li <nackli@163.com>
Copyright (c) 2024. All Rights Reserved.
*/
#include "pipe.h"
#include "FileFun.h"
int createPipe(PIPE_HANDLE hPipe[OPT_MAX])
{
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    return CreatePipe(&hPipe[OPT_READ], &hPipe[OPT_WRITE], &sa, 0);
#else	
    return pipe(hPipe);
#endif
}

static bool OnIsValidPipeName(const std::string& strPipe)
{
#ifdef _WIN32
    std::string prefix = "\\\\.\\pipe\\";
    if (strPipe.size() < prefix.size() ||
        strPipe.substr(0, prefix.size()) != prefix) {
        return false;
    }

    // 名称部分验证
    std::string strPipePart = strPipe.substr(prefix.size());
    if (strPipePart.empty() || strPipePart.size() > 256)
        return false;

    // 允许的字符: 字母数字、下划线、连字符、点
    std::regex nameRegex(R"(^[a-zA-Z0-9_.-]+$)");
    return std::regex_match(strPipePart, nameRegex);
#else
    // Unix命名管道通常为文件系统路径
    if (strPipe.empty() || strPipe.size() > 4096)
        return false;

    // 基本路径验证
    std::regex pathRegex(R"(^(/|(/[a-zA-Z0-9_.-]+)+)$)");
    if (!std::regex_match(strPipe, pathRegex)) {
        return false;
    }

    // 禁止特殊字符
    const std::string forbidden = " *?<>|&;!$(){}[]'\"\\";
    return strPipe.find_first_of(forbidden) == std::string::npos;
#endif
}

PIPE_HANDLE openNamePipe(const char* szFifo)
{
#ifdef _WIN32
    if (WaitNamedPipeA(szFifo, 20000))
    {
        PIPE_HANDLE hPipe = CreateFileA(szFifo,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
        DWORD mode = PIPE_READMODE_MESSAGE;
        SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);
        return hPipe;
    }
    return nullptr;
#else	
    return open(szFifo, O_RDONLY);
#endif
}

PIPE_HANDLE createNamePipe(const char* szPipeName)
{
#ifdef _WIN32
    return CreateNamedPipeA(szPipeName,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        0,
        0,
        NMPWAIT_WAIT_FOREVER,
        NULL);
#else	
    mkfifo(szPipeName, 0666);
    return open(fifo, O_WRONLY);
#endif
}

// 写入数据
bool writePipe(PIPE_HANDLE hWrite, const void* data, size_t size) {
    return writeDevice(hWrite, data, size);
}

// 读取数据
bool readPipe(PIPE_HANDLE hRead, void* buffer, size_t size) {
    return readDevice(hRead, buffer, size);
}

void closePipe(PIPE_HANDLE hPipe, const char* szPipeName)
{
#ifdef _WIN32
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
#else
    if (szPipeName)
        unlink(szPipeName);
#endif
    closeDevice(hPipe);
}

bool waitConnNamePipe(PIPE_HANDLE hPipe)
{
#ifdef _WIN32
    return ConnectNamedPipe(hPipe, nullptr);
#else
    return true;
#endif
}