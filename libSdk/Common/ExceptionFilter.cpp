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
		//ͨ�������쳣��ȡ��ջ
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
	//SEH��Windows �ṹ���쳣����������Win32 API
	::SetUnhandledExceptionFilter(UnhandledStructuredException);

	//C ����ʱ�� (CRT) �쳣������ CRT �ṩ���쳣������ơ�
	_set_purecall_handler(PureCallHandler);
	_set_new_handler(NewHandler);
	_set_invalid_parameter_handler(InvalidParameterHandler);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

	//C ����ʱ�źŴ����� CRT �ṩ���źŴ�����ơ�
	signal(SIGABRT, SigabrtHandler);
	signal(SIGINT, SigintHandler);
	signal(SIGTERM, SigtermHandler);
	signal(SIGILL, SigillHandler);
	//C++ ����ʱ�쳣����API�ɱ�׼���ṩ
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
	exit(1); // ����ʹ��������ʽ�˳�����
}
#endif

/*
	SIGHUP	1	��ֹ���̣� originally used to notify process of system terminal hangup
	SIGINT	2	�ж��źţ�ͨ���ɼ����ϵ��жϼ���Ctrl + C��������������ֹǰ̨����
	SIGQUIT	3	�˳��źţ�ͨ���ɼ����ϵ��˳�����Ctrl + \����������ֹ���̲���������ת���ļ�
	SIGILL	4	�Ƿ�ָ�����ִ���˷Ƿ�ָ��ʱ����
	SIGABRT	6	�����쳣��ֹ��ͨ����abort()�������ò���
	SIGBUS	7	Ӳ�����ߴ�����δ������ʣ�
	SIGFPE	8	�����쳣������������ѧ�������
	SIGKILL	9	ǿ����ֹ���̣������޷��������Ը��ź�
	SIGSEGV	11	�δ��󣬽�����ͼ����δ����򲻿ɷ��ʵ��ڴ�����ʱ����
	SIGPIPE	13	�ܵ����󣬽������ѹرյĹܵ�д������ʱ����
	SIGALRM	14	ʱ�Ӷ�ʱ�źţ����ڶ�ʱ����
	SIGTERM	15	��ֹ�źţ��������������ֹ�����̿��Բ��񲢽������������˳�
	SIGUSR1	30	�û��Զ����ź�1������Ӧ�ó����Զ����ͨ�Ż����
	SIGUSR2	31	�û��Զ����ź�2������Ӧ�ó����Զ����ͨ�Ż����
	SIGCHLD	17 / 18	�ӽ���״̬�ı䣬���ӽ�����ֹ��ֹͣʱ�򸸽��̷���
	SIGCONT	18 / 19	����ִ���źţ��ָ�����ͣ���̵�ִ��
	SIGSTOP	19 / 17	��ͣ����ִ�У������޷��������Ը��ź�
	SIGTSTP	20	��ͣ����ִ�У�ͨ���ɼ����ϵ���ͣ����Ctrl + Z������
	SIGWINCH	28	���ڴ�С�ı䣬���ն˴��ڴ�С�ı�ʱ����
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