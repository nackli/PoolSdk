/***************************************************************************************************************************************************/
/*
* @Author: Nack Li
* @version 1.0
* @copyright 2025 nackli. All rights reserved.
* @License: MIT (https://opensource.org/licenses/MIT).
* @Date: 2025-08-29
 * @LastEditTime: 2026-09-02 23:22:50
*/
/***************************************************************************************************************************************************/
#ifdef BACKTRACE_ENBALE
#include "ExceptionFilter.h"
#include <iostream>
#include <stdint.h>
#include <signal.h>

#include <sstream>
#include <limits>
#include <exception>
#define DUMP_FILE_NAME				"crash.dmp"

#ifdef _WIN32
#include <new.h>
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

	::SetUnhandledExceptionFilter(UnhandledStructuredException);


	_set_purecall_handler(PureCallHandler);
	_set_new_handler(NewHandler);
	_set_invalid_parameter_handler(InvalidParameterHandler);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);


	signal(SIGABRT, SigabrtHandler);
	signal(SIGINT, SigintHandler);
	signal(SIGTERM, SigtermHandler);
	signal(SIGILL, SigillHandler);

	set_terminate(TerminateHandler);
	set_unexpected(UnexpectedHandler);
}

static void DisableSetUnhandlerExcptionFilter()
{
	HMODULE hKernel = LoadLibraryA("kernel32.dll");
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
#include <execinfo.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <ucontext.h>

/* 提取崩溃点 PC/LR：不依赖栈展开，信号 trampoline 无 unwind 信息时也能精确定位 */
#if defined(__aarch64__)
	#define CRASH_PC(uc)  ((void*)(uc)->uc_mcontext.pc)
	#define CRASH_LR(uc)  ((void*)(uc)->uc_mcontext.regs[30])
#elif defined(__arm__)
	#define CRASH_PC(uc)  ((void*)(uc)->uc_mcontext.arm_pc)
	#define CRASH_LR(uc)  ((void*)(uc)->uc_mcontext.arm_lr)
#elif defined(__x86_64__)
	#define CRASH_PC(uc)  ((void*)(uc)->uc_mcontext.gregs[REG_RIP])
	#define CRASH_LR(uc)  ((void*)(uc)->uc_mcontext.gregs[REG_RBP])
#else
	#define CRASH_PC(uc)  (NULL)
	#define CRASH_LR(uc)  (NULL)
#endif

/* 以下辅助函数全部 async-signal-safe：
 * 只允许 write/纯算术/无分配调用；崩溃点可能在 malloc 锁内，
 * 一旦使用 printf/fopen/backtrace_symbols(内部malloc)/exit 会二次崩溃或死锁。 */
#define LIT(s) (s), sizeof(s) - 1

static void write_all(int fd, const char* buf, size_t len)
{
	while (len > 0)
	{
		ssize_t n = write(fd, buf, len);
		if (n <= 0)
			return;
		buf += n;
		len -= (size_t)n;
	}
}

static void write_u64_hex(int fd, unsigned long long v)
{
	char tmp[sizeof(v) * 2];
	int n = 0;
	do {
		tmp[n++] = "0123456789abcdef"[v & 0xF];
		v >>= 4;
	} while (v > 0);
	while (n > 0)
		write_all(fd, &tmp[--n], 1);	// 反序输出
}

static void write_u32_dec(int fd, unsigned v)
{
	char tmp[10];
	int n = 0;
	do {
		tmp[n++] = (char)('0' + v % 10);
		v /= 10;
	} while (v > 0);
	while (n > 0)
		write_all(fd, &tmp[--n], 1);
}

/* SA_SIGINFO 型 handler：ucontext 里的 arm_pc 就是崩溃指令地址，
 * 用 addr2line -e <app> -f -C 0x<pc> 即可直接定位崩溃函数 */
static void SignalHandler(int sig, siginfo_t* /*info*/, void* uctx)
{
	ucontext_t* uc = (ucontext_t*)uctx;
	int fd = open(DUMP_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		_exit(128 + sig);

	write_all(fd, LIT("signal="));
	write_u32_dec(fd, (unsigned)sig);
	write_all(fd, LIT(" pc="));
	write_u64_hex(fd, (unsigned long long)(uintptr_t)CRASH_PC(uc));
	write_all(fd, LIT(" lr="));
	write_u64_hex(fd, (unsigned long long)(uintptr_t)CRASH_LR(uc));
	write_all(fd, LIT("\nstack:\n"));

	/* 补充信息：通常栈会断在 sigreturn trampoline（无 unwind 信息），
	 * 仅当能展开时才有完整调用链，不作为定位依据 */
	void* array[16];
	int n = backtrace(array, 16);
	if (n > 0)
		backtrace_symbols_fd(array, n, fd);

	close(fd);
	/* _exit() 是 async-signal-safe 的；exit() 会跑静态析构导致二次崩溃 */
	_exit(128 + sig);
}
#endif

void initExceptionDump()
{
#ifdef _WIN32
	InstallUnexceptedExceptionHandler();
	DisableSetUnhandlerExcptionFilter();
#else
	/* SA_SIGINFO 才能拿到 ucontext，从而提取崩溃 PC；signal() 做不到 */
	static struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = SignalHandler;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	const int signalId[] = { SIGILL, SIGABRT, SIGFPE, SIGPIPE, SIGTERM, SIGSEGV, SIGBUS };
	for (size_t iIndex = 0; iIndex < sizeof(signalId) / sizeof(signalId[0]); iIndex++)
		sigaction(signalId[iIndex], &sa, NULL);
#endif
}
#endif