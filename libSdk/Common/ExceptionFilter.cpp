/*
Written by Nack li <nackli@163.com>
Copyright (c) 2024. All Rights Reserved.
*/

#include "ExceptionFilter.h"
#ifdef __PLATFORM_EXCEP_DUMP_H_
#include <iostream>
#include <stdint.h>
#include <signal.h>
#include <new.h>
#include <sstream>
#include <limits>
#include <exception>
#define DUMP_FILE_NAME				"crash.dmp"

#ifdef _WIN32
#include <windows.h>
#include <DbgHelp.h>
#include <minwinbase.h>
#pragma comment(lib,"Dbghelp.lib")
#pragma comment(lib,"User32.lib")

static int GenerateDump(EXCEPTION_POINTERS* exceptionPointers, const std::string& path)
{

	HANDLE hFile = ::CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInformation;
		minidumpExceptionInformation.ThreadId = GetCurrentThreadId();
		minidumpExceptionInformation.ExceptionPointers = exceptionPointers;
		minidumpExceptionInformation.ClientPointers = TRUE;
		bool isMiniDumpGenerated = MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hFile,
			MINIDUMP_TYPE::MiniDumpNormal,
			&minidumpExceptionInformation,
			nullptr,
			nullptr);
		CloseHandle(hFile);
		if (!isMiniDumpGenerated)
		{
			printf("MiniDumpWriteDump failed\n");
		}
	}
	else
	{
		printf("Failed to create dump file\n");
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

static void Snapshot(const std::string& path)
{
	__try
	{
		//通过触发异常获取堆栈
		RaiseException(0xE0000001, 0, 0, 0);
	}
	__except (GenerateDump(GetExceptionInformation(), path)) {}
}
#ifdef _WIN64
static LONG UnhandledStructuredException(struct _EXCEPTION_POINTERS* excp)
{
	GenerateDump(excp, DUMP_FILE_NAME);

	std::stringstream ss;
	ss << "IN UnhandledStructuredException.";
	ss << " ExceptionCode: 0x" << std::hex << excp->ExceptionRecord->ExceptionCode;
	ss << " ExceptionFlags:" << excp->ExceptionRecord->ExceptionFlags;

	//system(("cmd /K echo " + ss.str()).c_str());
	exit(EXIT_FAILURE);
}
#else
static LONG __stdcall UnhandledStructuredException(struct _EXCEPTION_POINTERS* excp)
{
	GenerateDump(excp, DUMP_FILE_NAME);
	//std::stringstream ss;
	//ss << "IN UnhandledStructuredException.";
	//ss << " ExceptionCode: 0x" << std::hex << excp->ExceptionRecord->ExceptionCode;
	//ss << " ExceptionFlags:" << excp->ExceptionRecord->ExceptionFlags;

	exit(EXIT_FAILURE);
}
#endif

static void PureCallHandler(void)
{
	Snapshot(DUMP_FILE_NAME);
	exit(EXIT_FAILURE);
}


static int NewHandler(size_t id) {
	Snapshot(DUMP_FILE_NAME);
	exit(EXIT_FAILURE);
	return 0;
}

static void InvalidParameterHandler(const wchar_t* expression,
	const wchar_t* function,
	const wchar_t* file,
	unsigned int line,
	uintptr_t pReserved)
{
	Snapshot(DUMP_FILE_NAME);
	exit(EXIT_FAILURE);
}

static void SigabrtHandler(int id) {
	Snapshot(DUMP_FILE_NAME);
	exit(EXIT_FAILURE);
}

static void SigintHandler(int id) {
	Snapshot(DUMP_FILE_NAME);
	exit(EXIT_FAILURE);
}

static void SigtermHandler(int id) {
	Snapshot(DUMP_FILE_NAME);
	exit(EXIT_FAILURE);
}

static void SigillHandler(int id) {
	Snapshot(DUMP_FILE_NAME);
	exit(EXIT_FAILURE);
}

static void TerminateHandler() {
	Snapshot(DUMP_FILE_NAME);
	exit(EXIT_FAILURE);
}

static void UnexpectedHandler() {
	Snapshot(DUMP_FILE_NAME);
	exit(EXIT_FAILURE);
}

static void InstallUnexceptedExceptionHandler()
{
	//SEH（Windows 结构化异常处理），属于Win32 API
	::SetUnhandledExceptionFilter(UnhandledStructuredException);

	//C 运行时库 (CRT) 异常处理，由 CRT 提供的异常处理机制。
	_set_purecall_handler(PureCallHandler);
	_set_new_handler(NewHandler);
	_set_invalid_parameter_handler(InvalidParameterHandler);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

	//C 运行时信号处理，由 CRT 提供的信号处理机制。
	signal(SIGABRT, SigabrtHandler);
	signal(SIGINT, SigintHandler);
	signal(SIGTERM, SigtermHandler);
	signal(SIGILL, SigillHandler);
	//C++ 运行时异常处理，API由标准库提供
	set_terminate(TerminateHandler);
	set_unexpected(UnexpectedHandler);
}

static void DisableSetUnhandlerExcptionFilter()
{
	HMODULE hKernel = LoadLibrary(L"kernel32.dll");
	if (!hKernel)
		return;
	void* addr = (void*)GetProcAddress(hKernel, "SetUnhandledExceptionFilter");

	if (addr)
	{
		unsigned char code[16];
		int size = 0;

		code[size++] = 0x48;
		code[size++] = 0x31;
		code[size++] = 0xC0;
		code[size++] = 0xC3;

		DWORD dwOldFlag, dwTempFlag;
		VirtualProtect(addr, size, PAGE_READWRITE, &dwOldFlag);
		WriteProcessMemory(GetCurrentProcess(), addr, code, size, NULL);
		VirtualProtect(addr, size, dwOldFlag, &dwTempFlag);
	}
}
#else

static void generate_stack_trace() {
	void* array[10];
	size_t size;
	char** strings;
	size_t i;

	size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);

	printf("Obtained %zd stack frames.\n", size);
	FILE* fr = fopen(DUMP_FILE_NAME, "w");
	if (fr)
	{
		for (i = 0; i < size; i++) {
			//printf("%s\n", strings[i]);
			fwrite(strings[i], 1, strlen(strings[i]), fr);
		}
		fclose(fr);
		fr = nullptr;
	}
	free(strings);
}

static void SignalHandler(int sig) {
	printf("Caught signal %d\n", sig);
	exit(1); // 或者使用其他方式退出程序
}
#endif

/*
	SIGHUP	1	终止进程， originally used to notify process of system terminal hangup
	SIGINT	2	中断信号，通常由键盘上的中断键（Ctrl + C）产生，用于终止前台进程
	SIGQUIT	3	退出信号，通常由键盘上的退出键（Ctrl + \）产生，终止进程并产生核心转储文件
	SIGILL	4	非法指令，进程执行了非法指令时发送
	SIGABRT	6	进程异常中止，通常由abort()函数调用产生
	SIGBUS	7	硬件总线错误（如未对齐访问）
	SIGFPE	8	浮点异常，如除以零等数学运算错误
	SIGKILL	9	强制终止进程，进程无法捕获或忽略该信号
	SIGSEGV	11	段错误，进程试图访问未分配或不可访问的内存区域时发送
	SIGPIPE	13	管道错误，进程向已关闭的管道写入数据时发送
	SIGALRM	14	时钟定时信号，用于定时操作
	SIGTERM	15	终止信号，用于请求进程终止，进程可以捕获并进行清理工作后退出
	SIGUSR1	30	用户自定义信号1，用于应用程序自定义的通信或控制
	SIGUSR2	31	用户自定义信号2，用于应用程序自定义的通信或控制
	SIGCHLD	17 / 18	子进程状态改变，当子进程终止或停止时向父进程发送
	SIGCONT	18 / 19	继续执行信号，恢复被暂停进程的执行
	SIGSTOP	19 / 17	暂停进程执行，进程无法捕获或忽略该信号
	SIGTSTP	20	暂停进程执行，通常由键盘上的暂停键（Ctrl + Z）产生
	SIGWINCH	28	窗口大小改变，当终端窗口大小改变时发送
*/
void initExceptionDump()
{
#ifdef _WIN32
	InstallUnexceptedExceptionHandler();
	DisableSetUnhandlerExcptionFilter();
#else
	const char signalId[] = { SIGILL,SIGABRT,SIGFPE,SIGPIPE,SIGTERM,SIGSEGV };
	for(int iIndex = 0;iIndex < sizeof(signalId) / sizeof(signalId);iIndex++)
		signal(signalId[iIndex], SignalHandler);
#endif
}
#endif