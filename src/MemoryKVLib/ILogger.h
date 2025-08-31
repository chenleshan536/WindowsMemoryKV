#pragma once

#include <string>

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void SetLogLevel(int logLevel)=0;
    virtual void Log(const wchar_t* message, int logLevel = 1, bool consolePrint = false)=0;
}; 