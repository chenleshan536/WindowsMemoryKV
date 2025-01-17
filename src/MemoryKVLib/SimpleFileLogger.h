#pragma once

#include <fstream>
#include <string>
#include <locale>

class SimpleFileLogger {
public:
    __declspec(dllexport) SimpleFileLogger(const wchar_t* loggerName);

    __declspec(dllexport) ~SimpleFileLogger();

    __declspec(dllexport) void SetLogLevel(int logLevel) { m_logLevel = logLevel; }

    __declspec(dllexport) void Log(const wchar_t* message, int logLevel = 1, bool consolePrint = false);
    
private:
    std::wofstream m_logFile; // Use wofstream for wide character output
    int m_logLevel{1};
    static void GetCurrentTime(std::wstring& time_str);
    static std::wstring GenerateFileName(const std::wstring& loggerName);
};

