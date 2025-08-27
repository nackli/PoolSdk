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
	virtual bool initOutMode(const char *) {return true;};
	virtual void closeOutPut(){};
};


class FileOutPutMode final:public OutPutMode
{
public:
	bool writeData(const std::string &strMsgData,int iLogLevel)
	{
		size_t iMsgSize = strMsg.length();
		if (n_hFile == nullptr)
			openCurrentFile();

		if (m_iCurrentSize + iMsgSize > m_iMaxSize)
			rotateFiles();

		if(FileSystem::writeFile(n_hFile, (void*)strMsg.c_str(), iMsgSize))
			m_iCurrentSize += iMsgSize;
	}
	bool initOutMode(const char *szFilName);
	{
		m_szBaseName = strdup(szFilName);
		std::thread tdSwitchFile(&FileOutPutMode::purgeOldFiles, this);
        tdSwitchFile.detach();
		return false;
	}	
	void closeOutPut()
	{
		if(m_frLog)
		{
			if(!FileSystem::closeFile(m_frLog))
				m_frLog = nullptr;
		}	
	}
	
private:
	void openCurrentFile()
	{
		OnCreateDirFromFilePath(m_szBaseName);	
		std::string strCurrentFile = generateFileName(m_iCurrentIndex);
		n_hFile = FileSystem::openOrCreateFile(m_strCurrentFile);

		FileSystem::fseekFile(n_hFile, 0, SEEK_END);
		m_iCurrentSize = FileSystem::getFileCurPos(n_hFile);
		
	}
	void rotateFiles()
	{
		m_iCurrentIndex++;
		m_iCurrentIndex = m_iCurrentIndex % 5000;
		openCurrentFile();
	    std::unique_lock<std::mutex>  lock(m_mtxSwitchLock);
		m_cvSwitchFile.notify_one();	
	}
	
	void OnCreateDirFromFilePath(const string strFilePath)
	{
		string strCfgDir = FileSystem::getDirFromFilePath(strFilePath);
		if (!FileSystem::IsDirectoryExists(strCfgDir))
			FileSystem::createDirectoryRecursive(strCfgDir);
	}

	void purgeOldFiles()
	{
		if (m_iMaxFiles <= 0) 
			return;
		while (1)
		{
			std::unique_lock<std::mutex>  lock(m_mtxSwitchLock);
			m_cvSwitchFile.wait(lock);

			map<uint64_t, string> mapFilePath;
	#ifdef _WIN32
			WIN32_FIND_DATAA findData;
			string strDir = FileSystem::getDirFromFilePath(m_strBaseName);
			HANDLE hFind = FindFirstFileA((m_strFilePrefix + "_*" + m_strFileExt).c_str(), &findData);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						uint64_t uTime = (static_cast<uint64_t>(findData.ftLastWriteTime.dwHighDateTime) << 32) | findData.ftLastWriteTime.dwLowDateTime;
						mapFilePath[uTime] = (strDir + findData.cFileName);
					}
				} while (FindNextFileA(hFind, &findData));
				FindClose(hFind);
			}

			size_t iIndex = mapFilePath.size();
			for (auto item : mapFilePath)
			{
				if (iIndex > (size_t)m_iMaxFiles)
				{
					if (DeleteFileA(item.second.c_str()) == 0)
						perror(("Error removing old log: " + item.second).c_str());
					iIndex--;
				}
				else
					break;
			}
	#else
			string strDir = FileSystem::getDirFromFilePath(m_strBaseName);
			//printf("ext length:%d\n",m_ext.length());
			if (strDir.empty())
				strDir = "./";

			DIR* dir = opendir(strDir.c_str());
			if (dir == NULL)
			{
				printf("[ERROR] %s is not a directory or not exist!", strDir.c_str());
				return;
			}

			struct dirent* d_ent = NULL;


			while ((d_ent = readdir(dir)) != NULL)
			{
				if ((strcmp(d_ent->d_name, ".") != 0) && (strcmp(d_ent->d_name, "..") != 0))
				{

					if (d_ent->d_type != DT_DIR)
					{
						//string d_name(d_ent->d_name);

						if (strcmp(d_ent->d_name + strlen(d_ent->d_name) - m_strFileExt.length(), m_strFileExt.c_str()) == 0)
						{
							struct stat statbuf;

							string strAbsolutePath;
							//string absolutePath = directory + string("/") + string(d_ent->d_name);
							if (strDir[strDir.length() - 1] == '/')
								strAbsolutePath = strDir + string(d_ent->d_name);
							else
								strAbsolutePath = strDir + "/" + string(d_ent->d_name);


							if (stat(strAbsolutePath.c_str(), &statbuf) != 0) {
								continue; // 
							}

							uint64_t uTime = (static_cast<uint64_t>(statbuf.st_mtim.tv_sec * 1000 + statbuf.st_mtim.tv_nsec / 1000000));
							mapFilePath[uTime] = strAbsolutePath;
						}
					}
				}
			}
			closedir(dir);

			size_t iIndex = mapFilePath.size();
			for (auto item : mapFilePath)
			{
				if (iIndex > (size_t)m_iMaxFiles)
				{
					if (remove(item.second.c_str()) != 0)
						perror(("Error removing old log: " + item.second).c_str());
					iIndex--;
				}
				else
					break;
			}
	#endif 
		}
	}	
private:
    std::condition_variable m_cvSwitchFile;
    std::mutex m_mtxSwitchLock;
	
	const char *m_szBaseName = nullptr；
	uint32_t m_iCurrentSize = 0;
	FILE *m_frLog = nullptr；
	
};


class UdpOutPutMode final:public OutPutMode
{
public:
    bool writeData(const std::string &strMsgData,int iLogLevel)
	{
		if(m_hSendSocket == INVALID_SOCKET)
			return false;
		struct sockaddr_in RecvAddr;
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(m_iNetPort);

		RecvAddr.sin_addr.s_addr = inet_addr(m_strNetIpAdd.c_str());
		if(sendto(m_hSendSocket,
			strMsg.c_str(), strMsg.length() , 0, (sockaddr*)&RecvAddr, sizeof(RecvAddr)) != SOCKET_ERROR)
			return true;
		return false;
	}
	bool initOutMode(const char *);
	{
	#ifdef _WIN32
		WSADATA wsaData;
		int  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != NO_ERROR) {
			throw std::runtime_error("WSAStartup failed with error\n");
			return false;
		}
		//---------------------------------------------
		// Create a socket for sending data
		m_hSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (m_hSendSocket == INVALID_SOCKET) {
			throw std::runtime_error("socket failed\n");
			WSACleanup();
			return false;
		}
	#else
		int m_hSendSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_hSendSocket < 0)
		{
			throw std::runtime_error("socket creation failed");
			return false;
		}
		return true;
	#endif
	}	
	
	void closeOutPut()
	{
	    if (m_hSendSocket != INVALID_SOCKET)
        {
#ifdef _WIN32
            closesocket(m_hSendSocket);
			WSACleanup();
#else
            close(m_hSendSocket);
#endif	
			m_hSendSocket = INVALID_SOCKET;
			
		}
	}
private:
#ifdef _WIN32
    SOCKET m_hSendSocket;
#else
    int m_hSendSocket;
#endif
	
};


class ConsoleOutPutMode final:public OutPutMode
{
public:
    bool writeData(const std::string &strMsgData,int iLogLevel)
	{
	#ifdef _WIN32
		const uint8_t clrIndex[] = { 0x0B,0x07,0x08,0x06,0x04,0x05 };
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), clrIndex[iLogLevel]);
		cout << strMsgData;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY);
	#else
		const char* clrIndex[] = { "/033[1;37m","/033[0;37m","/033[0;32;32m", "/033[1;33m","/033[0;32;31m","/033[1;32;31m" };
		printf("%s%s\033[0m", clrIndex[iLogLevel], strMsgData.c_str());
	#endif
		return true;
	}
};

#endif