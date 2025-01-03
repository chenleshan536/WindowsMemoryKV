#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include <codecvt>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

class SimpleFileLogger {
public:
    SimpleFileLogger(const wchar_t* loggerName);

    ~SimpleFileLogger();

    void Log(const wchar_t* message, bool consolePrint = true);
    static void GetCurrentTime(std::wstring& time_str);

private:
    std::wofstream m_logFile; // Use wofstream for wide character output

    static std::wstring GenerateFileName(const std::wstring& loggerName);
};

