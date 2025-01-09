#pragma once

#include <fstream>
#include <string>
#include <locale>

class SimpleFileLogger {
public:
    SimpleFileLogger(const wchar_t* loggerName);

    ~SimpleFileLogger();

    void Log(const wchar_t* message, bool consolePrint = false);
    static void GetCurrentTime(std::wstring& time_str);

private:
    std::wofstream m_logFile; // Use wofstream for wide character output

    static std::wstring GenerateFileName(const std::wstring& loggerName);
};

