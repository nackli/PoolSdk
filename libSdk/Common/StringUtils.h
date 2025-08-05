#pragma once
#ifndef __PLATFORM_STRING_UTILS_H__
#define __PLATFORM_STRING_UTILS_H__
#include <string>
#include <vector>
#include <sstream>
#include <string.h>
bool str2Bool(std::string strInput);
std::string &trimLeft(std::string &strInput, const std::string strTrim = " ");
std::string &trimRight(std::string &strInput, const std::string strTrim = " ");
std::string& subLeft(std::string& strInput, const std::string strTrim = " ");
std::string& subRight(std::string& strInput, const std::string strTrim = " ");
std::string &trim(std::string& strInput, const std::string strTrim = " ");
	
bool equals(const char* szLeft, const char* szRight, bool bCaseSensitive = true);
bool equals(const std::string& strLeft, const std::string& strRight, bool bCaseSensitive = true);
bool toLower(std::string &str);
bool toUpper(std::string &str);
bool startsWith(const std::string strValue, const std::string strStart, bool case_sensitive = true);
bool endsWith(const std::string strValue, const std::string strEnd, bool case_sensitive = true);

std::pair<std::string, std::string>spiltKv(const std::string strInput,char chDelimiter = '=');
std::string spiltLast(const std::string strInput, char chDelimiter = '.');
bool findStrExist(const std::string& strSource, const std::string& strFind);
char* replaceOne(const char* strSrc, const char* strOldSub, const char* strNewSub);
bool replaceOne(std::string& strDelimiter, const std::string& strFrom, const std::string& strTo);
void replaceAll(std::string& strSource, const std::string& strFrom, const std::string& strTo);
bool replaceLast(std::string& strData, const std::string& strFrom, const std::string& strTo);
void replaceAll(std::string& strSource, const char* szSrc, const char* szDest);

std::string getTypeName2WithDot(const std::string &strTypeName);

std::string ascii2HexString(const std::string& strInput);
std::string hexString2Ascii(const std::string& strInput);

std::string digit2Str(int lData, const int iBase = 10,const int iWidth = 0, const char chFill = '0');
std::string digit2Str(unsigned int lData, const int iBase = 10, const int iWidth = 0, const char chFill = '0');
std::string digit2Str(long lData, const int iBase = 10, const int iWidth = 0, const char chFill = '0');
std::string digit2Str(unsigned long lData, const int iBase = 10, const int iWidth = 0, const char chFill = '0');
std::string digit2Str(long long lData, const int iBase = 10, const int iWidth = 0, const char chFill = '0');
std::string digit2Str(unsigned long long lData, const int iBase = 10, const int iWidth = 0, const char chFill = '0');
std::string digit2Str(double lData, const int iBase = 100, const int iWidth = 0, const char chFill = '0', int iPrecision = 6);
std::string digit2Str(float lData, const int iBase = 100, const int iWidth = 0, const char chFill = '0', int iPrecision = 6);
std::string time2String(const std::string strFormat, const time_t theTime = -1);

int str2Int(std::string strInt);
unsigned int str2Uint(std::string strDigit);
long str2Long(std::string strDigit);
unsigned long str2Ulong(std::string strDigit);
long long str2LongLong(std::string strDigit);
unsigned long long str2ULongLong(std::string strDigit);
double str2Double(std::string strDigit);
float str2Float(std::string strDigit);
time_t string2Timer(const std::string strFormat,const std::string strTimer);
	

std::string toBase64(const char* szByteData, size_t uLen = 0);
std::string fromBase64(const std::string& strBase64);
bool IsBase64(unsigned char c);
	
std::wstring UTF8ToUnicode(const std::string str);//setlocale(LC_ALL, "zh_cn")
std::string UnicodeToUTF8(const std::wstring wstr);//setlocale(LC_ALL, "zh_cn")
std::string UnicodeToANSI(const std::wstring& wstr);//setlocale(LC_ALL, "zh_cn")
std::wstring ANSIToUnicode(const std::string& str);//setlocale(LC_ALL, "zh_cn")
std::string UTF8ToANSI(const std::string& str);//setlocale(LC_ALL, "zh_cn")
std::string ANSIToUTF8(const std::string& str);//setlocale(LC_ALL, "zh_cn")
std::string GbkToUtf8(const std::string& str);
std::string Utf8ToGbk(const std::string& str);
bool OnIsExistGBK(const std::string & strInPut);


std::string generateUUID();
std::string randomString(size_t length);
int randomInt(int min, int max);
double random_double(double min, double max);

template < typename T>
T transFomat(const std::string& str);

template < typename T>
std::vector<T> split(const std::string& strInput, const std::string& strDelimiter);

template<typename T>
std::vector<T> split(const std::string& str, char chDelimiter = ':');

template < typename T>
std::vector<T> splitAndTrim(const std::string& strInput, const std::string& strDelimiter);

template < typename T>
std::vector<T> splitAndTrim(const std::string& str, char chDelimiter);

template < typename T>
T getSplitData(const std::string& str, char chDelimiter,int iIndex = -1);

template < typename T>
T getSplitData(const std::string& str, const std::string& strDelimiter,int iIndex = -1);

template < typename T>
std::string join(const std::vector<T>& v, const std::string& token);

template<typename T>
std::string toString(T value);

template < typename T>
T getEnv(const char* name, T defaultVal);

template<typename T, typename Fun >
std::vector<T> splitTransformed(std::string str, const std::string& delimiter, Fun transformation);

template < typename T, typename Fun>
std::vector<T> splitTransformed(const std::string& str, char chDelimiter, Fun transformation);

template < typename T>
T transFomat(const std::string& str)
{
	T result;
	if (str.empty())
		return T();
	std::istringstream(str) >> result;
	return result;
}

template<> inline
std::string transFomat(const std::string& str)
{
	return str;
}

template<> inline
const char* transFomat(const std::string& str)
{
#ifdef _WIN32	
	return _strdup(str.c_str());//free
#else
	return strdup(str.c_str());//free
#endif
}

template<> inline
char* transFomat(const std::string& str)
{
#ifdef _WIN32	
	return _strdup(str.c_str());//free
#else
	return strdup(str.c_str());//free
#endif
}

template < typename T>
std::string join(const std::vector<T> &v, const std::string & strToken)
{
	std::ostringstream result;
	for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); i++) 
	{
		if (i != v.begin())
			result << strToken;
		result << *i;
	}               
	return result.str();
}

template < typename T>
std::string toString(T value)
{
	std::ostringstream ss;
	ss << value;
	return ss.str();
}

template < typename T>
T getEnv(const char* name, T defaultVal)
{
	char* szValue = nullptr;
#ifdef Win32	
	size_t sz = 0;
	if (_dupenv_s(&szValue, &sz, name) != 0 || !szValue)
#else
	szValue = getenv(name);
	if (!szValue)
#endif	
		return defaultVal;
	std::string strValue = std::string();
	
	if (szValue)
	{
		strValue = std::move(szValue);
		free(szValue);
		szValue = nullptr;
	}
	return transFomat<T>(strValue);
}

template<typename T,typename Fun>
std::vector<T>  splitTransformed(std::string str, const std::string& delimiter, Fun transformation)
{
	std::vector<T> result;
	if (delimiter.empty()) 
	{
		for (auto c : str) 
			result.emplace_back(transformation(std::string(1, c)));
		return result;
	}

	size_t pos = str.find(delimiter);
	if (pos == std::string::npos) 
	{
		result.emplace_back(transformation(str));
		return result;
	}
	while (pos != std::string::npos) 
	{
		std::string strSub = str.substr(0, pos);

		if(!strSub.empty())
			result.emplace_back(transformation(strSub));

		str = str.substr(pos + delimiter.size());
		pos = str.find(delimiter);
	}
	result.emplace_back(transformation(str));
	return result;
}

template<typename T, typename Fun>
std::vector<T> splitTransformed(const std::string& strInput, char chDelimiter, Fun fun)
{
	std::vector<T> elems;
	std::istringstream strStream(strInput);
	std::string strItem;
	while (std::getline(strStream, strItem, chDelimiter))
	{
		if(!strItem.empty())
			elems.emplace_back(fun(strItem));
	}
	return elems;
}

template<typename T>
std::vector<T> split(const std::string& strInput, const std::string& strDelimiter)
{
	return splitTransformed<T>(strInput, strDelimiter, &transFomat<T>);
}

template < typename T>
std::vector<T> split(const std::string& strInput, char chDelimiter)
{
	return splitTransformed<T>(strInput, chDelimiter, &transFomat<T>);
}

template<typename T>
std::vector<T> splitAndTrim(const std::string& strInput, const std::string& strDelimiter) 
{
	return splitTransformed<T>(strInput, strDelimiter, [](std::string strInput) {return transFomat<T>(trim(strInput, " ")); });
}

template < typename T>
std::vector<T> splitAndTrim(const std::string& strInput, char chDelimiter)
{
	return splitTransformed<T>(strInput, chDelimiter, [](std::string strInput) {return transFomat<T>(trim(strInput, " ")); });
}


template < typename T>
T getSplitData(const std::string& str, char chDelimiter, int iIndex)
{
	if (str.empty())
		return T();
	std::vector<T> vectorT = split<T>(str, chDelimiter);
	if (!vectorT.empty())
	{
		if (iIndex == -1)
			iIndex = vectorT.size() - 1;
		else if (iIndex < 0 || iIndex >= (int)vectorT.size())
			return T();
		return vectorT[iIndex];
	}
	return T();
}

template < typename T>
T getSplitData(const std::string& str, const std::string& strDelimiter, int iIndex)
{
	if (str.empty() || strDelimiter.empty())
		return T();
	std::vector<T> vectorT = split<T>(str, strDelimiter);

	if (!vectorT.empty())
	{
		if (iIndex == -1)
			iIndex = vectorT.size() - 1;
		else if (iIndex < 0 && iIndex >= (int)vectorT.size())
			return T();
		return vectorT[iIndex];
	}

	return T();
}
#endif


