#pragma once
#include <memory>
#include <ctime>
#include <cassert>
#include <vector>
#include <thread>
#include "fmt/format.h"
#include "fmt/base.h"
#include "LoggerLevel.h"
 

using memory_buf_t = fmt::memory_buffer;
using string_view_t = fmt::basic_string_view<char>;

template<typename T>
inline unsigned int CntDigits(T n)
{
    using typeCnt = typename std::conditional<(sizeof(T) > sizeof(uint32_t)), uint64_t, uint32_t>::type;
    return static_cast<unsigned int>(fmt::
        // fmt 7.0.0 renamed the internal namespace to detail.
        // See: https://github.com/fmtlib/fmt/issues/1538
#if FMT_VERSION < 70000
        internal
#else
        detail
#endif
        ::count_digits(static_cast<typeCnt>(n)));
}

template<typename T>
inline void uint2Str(T n, unsigned int width, memory_buf_t& bufDest)
{
    for (auto digits = count_digits(n); digits < width; digits++)
        bufDest.push_back('0');
    append_int(n, bufDest);
}

inline void append_int(uint64_t n, memory_buf_t& bufDest)
{
    fmt::format_int iData(n);
    bufDest.append(iData.data(), iData.data() + iData.size());
}

inline void append_int_100(int n, memory_buf_t& bufDest)
{
    if (n >= 0 && n < 100) // 0-99
    {
        bufDest.push_back(static_cast<char>('0' + n / 10));
        bufDest.push_back(static_cast<char>('0' + n % 10));
    }
    else // unlikely, but just in case, let fmt deal with it
    {
        fmt::format_to(bufDest.begin(), "{:02}", n);
    }
}

inline void append_int_1000(int n, memory_buf_t& bufDest)
{
    if (n < 1000)
    {
        bufDest.push_back(static_cast<char>(n / 100 + '0'));
        n = n % 100;
        bufDest.push_back(static_cast<char>((n / 10) + '0'));
        bufDest.push_back(static_cast<char>((n % 10) + '0'));
    }
    else
    {
        append_int(n, bufDest);
    }
}

inline void append_string_view(string_view_t view, memory_buf_t& bufDest)
{
    bufDest.append(view.data(), view.data() + view.size());
}

inline string_view_t to_string_view(const memory_buf_t& buf)
{
    return string_view_t{ buf.data(), buf.size() };
}


struct LogMessage {
#ifdef _WIN32
    SYSTEMTIME tmCreate;// creation time
#else
    struct tm_milli
    {
        struct tm tmTime;
        uint16_t milli;
    };
    tm_milli tmCreate;
#endif
    int iLevLog;// log level
    std::thread::id iThreadId;// thread id
    const char * szFileName = nullptr;// source file name
    size_t iLineNo = 0;// source line number
    const char *szMsgCtx =nullptr;// log message
    const char * strLogName = nullptr;// logger name
    const char* szFunName = nullptr;// logger name
    LogMessage(int level, const char* szFile, size_t iLine, const char* szMsg,  const char* szFName)
        : iLevLog(level), iThreadId(std::this_thread::get_id()), szFunName(szFName),
        szFileName(szFile), iLineNo(iLine), szMsgCtx(szMsg), strLogName(nullptr)
    {
#ifdef _WIN32
        GetLocalTime(&tmCreate);
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tmCreate.tmTime);
        tmCreate.milli = tv.tv_usec / 1000;
#endif
    }
};

class Format
{
public:
    using ptr = std::unique_ptr<Format>;
    virtual void format(const LogMessage& msg, memory_buf_t & dest) = 0;
};
 
// 具体的日志格式
class MsgFormat final : public Format
{
public:
    void format(const LogMessage& msg, memory_buf_t & bufDest) 
    {
        if(msg.szMsgCtx)
            append_string_view(msg.szMsgCtx, bufDest);
    }
};
 
class LevelFormat final : public Format
{
public:
    void format(const LogMessage& msg, memory_buf_t& dest) override
    {
        append_string_view(::getLoggerLevelName(msg.iLevLog) , dest);
    }
};

class LevelShortFormat final : public Format
{
public:
    void format(const LogMessage& msg, memory_buf_t& dest) override
    {
        append_string_view(::getLoggerLevelShortName(msg.iLevLog), dest);
    }
};

class TimeFormat final: public Format
{
public:
    TimeFormat(const std::string &format = "%H:%M:%S") : formatTime(format)
    {
    }
    void format(const LogMessage &msg, memory_buf_t& dest) override
    {
#if _WIN32
        append_int(msg.tmCreate.wYear, dest);
        dest.push_back('-');

        append_int_100(msg.tmCreate.wMonth, dest);
        dest.push_back('-');

        append_int_100(msg.tmCreate.wDay, dest);
        dest.push_back(' ');

        append_int_100(msg.tmCreate.wHour, dest);
        dest.push_back(':');

        append_int_100(msg.tmCreate.wMinute, dest);
        dest.push_back(':');

        append_int_100(msg.tmCreate.wSecond, dest);
        dest.push_back('.');
  
        append_int_1000(msg.tmCreate.wMilliseconds, dest);
#else
        append_int(msg.tmCreate.tmTime.tm_year + 1900, dest);
        dest.push_back('-');

        append_int_100(msg.tmCreate.tmTime.tm_mon + 1, dest);
        dest.push_back('-');

        append_int_100(msg.tmCreate.tmTime.tm_mday, dest);
        dest.push_back(' ');

        append_int_100(msg.tmCreate.tmTime.tm_hour, dest);
        dest.push_back(':');

        append_int_100(msg.tmCreate.tmTime.tm_min, dest);
        dest.push_back(':');

        append_int_100(msg.tmCreate.tmTime.tm_sec, dest);
        dest.push_back('.');
  
        append_int_1000(msg.tmCreate.milli, dest);
#endif
    }
 
private:
    std::string formatTime;
};
class FunctionFormat final: public Format
{
public:
    FunctionFormat()
    {
    }
    void format(const LogMessage& msg, memory_buf_t& dest) override
    {
        if(msg.szFunName)
            append_string_view(msg.szFunName, dest);
    }
};
class FileFormat final: public Format
{
public:
    void format(const LogMessage &msg, memory_buf_t& bufDest) override
    {
        if (msg.szFileName)
           append_string_view(msg.szFileName, bufDest);
    }
};
class LineFormat final: public Format
{
public:
    void format(const LogMessage &msg, memory_buf_t &bufDest) override
    {
        append_int(msg.iLineNo, bufDest);
    }
};

class FileNameAndLineFormat final : public Format
{
public:
    void format(const LogMessage& msg, memory_buf_t& bufDest) override
    {
        if(msg.szFileName)
            append_string_view(msg.szFileName, bufDest);
        bufDest.push_back(':');
        append_int(msg.iLineNo, bufDest);
    }
};

class ThreadIdFormat final : public Format
{
public:
    void format(const LogMessage &msg, memory_buf_t & bufDest) override
    {
        append_int(static_cast<int>(reinterpret_cast<uintptr_t>(&msg.iThreadId)), bufDest);
    }
};
class TableFormat final : public Format
{
public:
    void format(const LogMessage &msg, memory_buf_t & bufDest) override
    {
        bufDest.push_back('\t');
        //append_string_view("\t", bufDest);
    }
};
class NewLineFormat final : public Format
{
public:
    void format(const LogMessage &msg, memory_buf_t & bufDest) override
    {
        bufDest.push_back('\n');
    }
};
class CharFormat final : public Format
{
public:
    explicit CharFormat(char ch)
        : m_chData(ch)
    {}

    void format(const LogMessage& msg, memory_buf_t& bufDest) override
    {
        bufDest.push_back(m_chData);
    }

private:
    char m_chData;
};

class OtherFormat final : public Format
{
public:
    OtherFormat(const std::string &other = "") : other(other)
    {
    }
    void format(const LogMessage &msg, memory_buf_t & bufDest) override
    {
        append_string_view(other, bufDest);
    }
 
private:
    std::string other;
};
/*
%Y %D %t：Indicates the date
%t：Indicates the thread ID
%@：Indicate the absolute path and line number of the source code
%l：Indicates the log level
%v：Indicate log content
%T：Indicate tab indentation
%g：Indicate file name
%n：Indicate line break
%!：Indicate function name
%#：Indicate line numbers
*/
class FormatterBuilder // Log format constructor
{
public:
    FormatterBuilder(const std::string &pattern = "[%D] [tid:%t] [%l] [%@]  %v%n") : m_strPattern(pattern)
    {
        parsePattern();
    }

    void format(memory_buf_t &bufDest, const LogMessage &msg)
    {
        for (auto &f : m_vFormatsPtr)
        {
            f->format(msg, bufDest);
        }
    }

    std::string format(const LogMessage &msg)
    {
        memory_buf_t bufDest;
        format(bufDest, msg);
        return std::string(bufDest.data(), bufDest.size());
    }
        
    void updatePattern(const std::string& strPattern)
    {
        if (strPattern.empty())
            return;
        m_vFormatsPtr.clear();
        m_strPattern = strPattern;
        parsePattern();
    }
private:
    bool parsePattern()
    {
        size_t pos = 0;                                         // Indicate the processed location
        char key;                                               // Indicate formatting keywords
        std::string value;                                      // Representing formatted values
        std::vector<std::pair<char, std::string>> items;        // Representing formatting items
        while (pos < m_strPattern.size())
        {
            // 1. If it's not%, it means it's a regular character
            while (pos < m_strPattern.size() && m_strPattern[pos] != '%')
            {
                value += m_strPattern[pos];
                pos++;
            }
            // 2.If it is%, it means it is a formatted character
            if (pos + 1 < m_strPattern.size() && m_strPattern[pos + 1] == '%')
            {
                // 2.1  If it is%%, it means it is%
                value += m_strPattern[pos];
                pos += 2;
            }
            if (!value.empty())
            {
                items.emplace_back(std::make_pair(' ', value));
                value.clear();
                continue;
            }
            // 3. If it is a formatted character
            if (pos < m_strPattern.size() && m_strPattern[pos] == '%')
            {
                pos++; // skip %
                // Merge check: skip '%' and perform a unified check to see if it exceeds the string range
                if (pos >= m_strPattern.size())
                {
                    std::cerr << "Error: Unexpected end of pattern after % at position " << pos << "." << std::endl;
                    return false;
                }
                key = m_strPattern[pos];
                pos++;
                if (pos < m_strPattern.size() && m_strPattern[pos] == '{')
                {
                    pos++;
                    // 3.1 If it is {, it means it is a sub format
                    while (pos < m_strPattern.size() && m_strPattern[pos] != '}')
                    {
                        value += m_strPattern[pos];
                        pos++;
                    }
                    if (pos >= m_strPattern.size() || m_strPattern[pos] != '}')
                    {
                        std::cerr << "Error: Missing closing brace for sub - pattern starting at position " << pos - value.length() - 1 << "." << std::endl;
                        return false;
                    }
                    pos++; 
                }
 
                //// Check if it is an unfamiliar formatted keyword
                //static const std::unordered_set<char> validKeys = {'D','Y','y' ,'#','t', 'g', 'l', '!', 'T', 'v', '%', 'n','@','%'};
                //if (validKeys.find(key) == validKeys.end())
                //{
                //    std::cerr << "Error: Unknown format keyword '" << key << "' at position " << pos - sizeof(char) - 1 << "." << std::endl;
                //    return false;
                //}
 
                items.emplace_back(std::make_pair(key, value));
                value.clear();
            }
        }
        // 4. Create formatted object
        for (auto &item : items)
        {
            m_vFormatsPtr.push_back(createItem(item.first, item.second));
        }
        return true;
    }
 
    // 创建格式化对象
    Format::ptr createItem(const char &key, const std::string &value)
    {
        switch (key)
        {
        case ('Y'):
        case ('y'):
        case ('D'):
            return std::make_unique <TimeFormat>(value);
        case ('t'):
            return std::make_unique <ThreadIdFormat>();
            break;
        case ('!'):
            return std::make_unique <FunctionFormat>();
        case ('g'):
                return std::make_unique <FileFormat>();
        case ('#'):
            return std::make_unique <LineFormat>();
        case ('l'):
            return std::make_unique <LevelFormat>();
        case ('L'):
            return std::make_unique <LevelShortFormat>();
        case ('T'):
            return std::make_unique <TableFormat>();
        case ('v'):
            return std::make_unique <MsgFormat>();
        case ('n'):
            return std::make_unique <NewLineFormat>();
        case ('@'):
            return std::make_unique <FileNameAndLineFormat>();
        case ('%'):
            return std::make_unique <CharFormat>('%');

        default:
            return std::make_unique <OtherFormat>(value);
        } 
    }
 
private:
    std::string m_strPattern; // 格式化字符串
    std::vector<Format::ptr> m_vFormatsPtr;
};