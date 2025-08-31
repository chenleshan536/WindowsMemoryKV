#pragma once

#include "../MemoryKVLib/ILogger.h"
#include <thread>
#include <string>
#include <vector>

class MockLogger : public ILogger {
public:
    bool m_debug;

    MockLogger(bool debug=false) : m_debug(debug)
    {
    }

    void SetLogLevel(int logLevel) override
    {
        if (m_debug)
            std::cout << "["<< std::this_thread::get_id() << "] set log level " << logLevel << std::endl;
    }
    void Log(const wchar_t* message, int logLevel = 1, bool consolePrint = false)
    {
        if(m_debug)
            std::wcout << L"[" << std::this_thread::get_id() << L"] loglevel="<<logLevel<<": "<< message << std::endl;
    }
}; 