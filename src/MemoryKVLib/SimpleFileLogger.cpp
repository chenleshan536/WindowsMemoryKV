#include "SimpleFileLogger.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

SimpleFileLogger::SimpleFileLogger(const wchar_t* loggerName)
{
    std::wstring file_name = GenerateFileName(loggerName);
    m_logFile.open(file_name, std::ios_base::out | std::ios_base::app);
    if (!m_logFile.is_open()) {
        throw std::runtime_error("Unable to open log file");
    }
}

SimpleFileLogger::~SimpleFileLogger()
{
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void SimpleFileLogger::Log(const wchar_t* message, bool consolePrint)
{
    std::wstring time_str;
    GetCurrentTime(time_str);
    if (m_logFile.is_open()) {
        m_logFile << time_str << " - " << message << std::endl;
    }
    if (consolePrint)
        std::wcout << time_str << " - " << message << std::endl;
}

void SimpleFileLogger::GetCurrentTime(std::wstring& time_str)
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = {};
    localtime_s(&now_tm, &in_time_t);

    std::chrono::duration<long long, std::ratio<1, 10000000>> duration = now.time_since_epoch();
    std::chrono::_Duration_div_mod1<long long, std::ratio<1, 1000>, int, true>::type milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;

    std::wstringstream wss;
    wss << std::put_time(&now_tm, L"%Y%m%d_%H%M%S")
    << L"." << std::setfill(L'0') << std::setw(3) << milliseconds.count();

    time_str = wss.str();
}

std::wstring SimpleFileLogger::GenerateFileName(const std::wstring& loggerName)
{
    std::wstring time_str;
    GetCurrentTime(time_str);
    return loggerName + L"_" + time_str + L".log";
}
