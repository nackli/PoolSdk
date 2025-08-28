#pragma once
#ifndef __OUT_PUT_MODE_H__
#define __OUT_PUT_MODE_H__

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib,"Ws2_32.lib")
#else
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<arpa/inet.h>
#include <thread>
#include <unistd.h>
#endif
#include <string>
#include <mutex>
#include <map>


class OutPutMode
{
public:
    using ptr = std::unique_ptr<OutPutMode>;
    virtual bool writeData(const std::string &strMsgData,int iLogLevel) = 0;
	virtual bool initOutMode(const char *,int iMaxSize = 0) {return true;};
	virtual void closeOutPut(){};
	virtual void setMaxFileNum(int iFileNum) {};
	virtual void flushFile() {};
	virtual void changeOutModeCfg(const std::string &) {};
};


class FileOutPutMode final:public OutPutMode
{
public:
	bool writeData(const std::string& strMsgData, int iLogLevel);
	bool initOutMode(const char *szFilName,int iMaxSize = 0);	
	void closeOutPut();
	void setMaxFileNum(int iFileNum);
	void flushFile();
	void changeOutModeCfg(const std::string&);
private:
	void openCurFile();
	void changeNextFiles();
	void purgeOldFiles();
	void parseFileNameComponents();
	int findMaxFileIndex();
private:
    std::condition_variable m_cvSwitchFile;
    std::mutex m_mtxSwitchLock;
	uint32_t m_iCurrentSize = 0;
	uint32_t m_iCurrentIndex = 0;
	FILE* m_frLog = nullptr;
	int m_iMaxSize = 0;
	int m_iMaxFileNum = 0;
	std::string m_strBaseName;
	std::string m_strFilePrefix;
	std::string m_strFileExt;
};


class UdpOutPutMode final:public OutPutMode
{
public:
	bool writeData(const std::string& strMsgData, int iLogLevel);
	bool initOutMode(const char *szAddr, int iPort);
	void closeOutPut();
private:
	std::string m_strNetAddr;
	int m_iNetPort = 0;
#ifdef _WIN32
    SOCKET m_hSendSocket = 0;
#else
    int m_hSendSocket = 0;
#endif
	
};


class ConsoleOutPutMode final:public OutPutMode
{
public:
	bool writeData(const std::string& strMsgData, int iLogLevel);
};

#endif