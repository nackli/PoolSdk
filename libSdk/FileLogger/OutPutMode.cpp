#include "OutPutMode.h"
#include "Common/FileSystem.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#ifndef INVALID_SOCKET
#define INVALID_SOCKET                  0x0
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR            (-1)
#endif
/*************************************************************************************************************************/
using namespace std;
static inline void OnCreateDirFromFilePath(const string strFilePath)
{
	string strCfgDir = FileSystem::getDirFromFilePath(strFilePath);
	if (!FileSystem::IsDirectoryExists(strCfgDir))
		FileSystem::createDirectoryRecursive(strCfgDir);
}

static inline void processFileName(const std::string& fileName, vector<int>& indices)
{
	size_t pos = fileName.find_last_of("_");
	if (pos != string::npos)
	{
		const std::string numStr = fileName.substr(pos + 1);
		if (!numStr.empty())
			indices.push_back(std::stoi(numStr));
	}
}
/*************************************************************************************************************************/

bool FileOutPutMode::writeData(const std::string &strMsgData,int iLogLevel)
{
	size_t iMsgSize = strMsgData.length();
	if (m_frLog == nullptr)
		openCurFile();

	if (m_iCurrentSize + iMsgSize > (size_t)m_iMaxSize)
		changeNextFiles();

	if (FileSystem::writeFile(m_frLog, (void*)strMsgData.c_str(), iMsgSize))
	{
		m_iCurrentSize += iMsgSize;
		return true;
	}
	return false;

}

bool FileOutPutMode::initOutMode(const char *szFilName,int iMaxSize)
{
	changeOutModeCfg(szFilName);
	m_iMaxSize = iMaxSize;
	std::thread tdSwitchFile(&FileOutPutMode::purgeOldFiles, this);
	tdSwitchFile.detach();
	return false;
}


void FileOutPutMode::changeOutModeCfg(const std::string& strCfgName)
{
	if (!strCfgName.empty())
	{
		m_strBaseName = strCfgName;
		parseFileNameComponents();
		m_iCurrentIndex = findMaxFileIndex();
	}
}

void FileOutPutMode::setMaxFileNum(int iFileNum)
{
	m_iMaxFileNum = iFileNum;
}

void FileOutPutMode::flushFile()
{
	FileSystem::flushFile(m_frLog);
}

void FileOutPutMode::closeOutPut()
{
	if(m_frLog)
	{
		if(!FileSystem::closeFile(m_frLog))
			m_frLog = nullptr;
	}	
}
	
void FileOutPutMode::openCurFile()
{
	std::string strCurrentFile = m_strFilePrefix + "_" + std::to_string(m_iCurrentIndex) + m_strFileExt;

	m_frLog = FileSystem::reopenOrCreateFile(strCurrentFile.c_str(), m_frLog);

	FileSystem::fseekFile(m_frLog, 0, SEEK_END);
	m_iCurrentSize = FileSystem::getFileCurPos(m_frLog);
}

void FileOutPutMode::changeNextFiles()
{
	m_iCurrentIndex++;
	m_iCurrentIndex = m_iCurrentIndex % 5000;
	openCurFile();
	std::unique_lock<std::mutex>  lock(m_mtxSwitchLock);
	m_cvSwitchFile.notify_one();	
}

void FileOutPutMode::parseFileNameComponents()
{
	if (m_strBaseName.empty())
		return;
	m_strFilePrefix = m_strBaseName;
	m_strFileExt = "";
	size_t lastDot = m_strBaseName.rfind('.');

	if (lastDot != std::string::npos) {
		m_strFilePrefix = m_strBaseName.substr(0, lastDot);
		m_strFileExt = m_strBaseName.substr(lastDot);
	}
}

int FileOutPutMode::findMaxFileIndex()
{
	std::vector<int> vecIndices;
#if 0
#ifdef _WIN32
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(/*(m_strFilePrefix + "_*" + m_strFileExt).c_str()*/"./Data/log/*.log", &findData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				processFileName(findData.cFileName, vecIndices);
			}
		} while (FindNextFileA(hFind, &findData));
		FindClose(hFind);
	}
#else
	string strDir = FileSystem::getDirFromFilePath(m_strBaseName);
	DIR* dir = opendir(strDir.c_str());
	if (dir) {
		struct dirent* ent = nullptr;
		while ((ent = readdir(dir)) != nullptr) {
			if (ent->d_type == DT_REG) {
				processFileName(ent->d_name, vecIndices);
			}
		}
		closedir(dir);
	}
#endif
#else
	string strRegFile = m_strFilePrefix + "_";
	FileSystem::FileInfoList listFile = FileSystem::getFilesInCurDir(strRegFile, m_strFileExt,true);
	for(auto it = listFile.begin();it != listFile.end();it ++)
		processFileName(it->strFileName, vecIndices);
#endif
	return vecIndices.empty() ? 0 : *std::max_element(vecIndices.begin(), vecIndices.end());
}

void FileOutPutMode::purgeOldFiles()
{
	if (m_iMaxFileNum <= 0)
		return;
	while (1)
	{
		std::unique_lock<std::mutex>  lock(m_mtxSwitchLock);
		m_cvSwitchFile.wait(lock);
#if 1
		string strRegFile = m_strFilePrefix + "_";
		FileSystem::FileInfoList listFile = FileSystem::getFilesInCurDir(strRegFile, m_strFileExt);
		size_t iIndex = listFile.size();

		for (auto item : listFile)
		{
			if (iIndex > (size_t)m_iMaxFileNum)
			{
				if (!FileSystem::delFile(item.strFileName.c_str()))
					perror(("Error removing old log: " + item.strFileName).c_str());
				iIndex--;
			}
			else
				break;
		}
#else
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
			if (iIndex > (size_t)m_iMaxFileNum)
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
#endif
	}

}	

//UdpOutPutMode
bool UdpOutPutMode::writeData(const std::string &strMsgData,int iLogLevel)
{
	if(m_hSendSocket == INVALID_SOCKET)
		return false;
	struct sockaddr_in RecvAddr;
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(m_iNetPort);

	RecvAddr.sin_addr.s_addr = inet_addr(m_strNetAddr.c_str());
	if(sendto(m_hSendSocket,
		strMsgData.c_str(), strMsgData.length() , 0, (sockaddr*)&RecvAddr, sizeof(RecvAddr)) != SOCKET_ERROR)
		return true;
	return false;
}
bool UdpOutPutMode::initOutMode(const char * szAddr, int iPort)
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
#endif
	m_strNetAddr = szAddr;
	m_iNetPort = iPort;
	return true;
}	

void UdpOutPutMode::closeOutPut()
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
	

bool ConsoleOutPutMode::writeData(const std::string &strMsgData,int iLogLevel)
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