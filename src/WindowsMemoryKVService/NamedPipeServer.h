#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <functional>
#include <sstream>
#include "..\MemoryKVLib\SimpleFileLogger.h"
#define PIPE_NAME L"\\\\.\\pipe\\TaskPipe2"


using NamedPipeServerCallback = std::function<bool(const std::wstring&)>;

class NamedPipeServer {
private:
    std::atomic<bool> m_running;
    SimpleFileLogger& m_logger;    
    NamedPipeServerCallback m_callback;
    bool CallCallback(const std::wstring& message) const;
    bool CreateNamedPipeObject(HANDLE& hPipe) const;
    static std::wstring StringToWstring(const std::string& str);
    bool HandleClientRequest(HANDLE hPipe) const;
public:
    NamedPipeServer(SimpleFileLogger& rlogger);
    void SetCallback(NamedPipeServerCallback callback);
    /**
     * \brief listen to named pipe client and handle client request till stop
     */
    void Serve();

};
