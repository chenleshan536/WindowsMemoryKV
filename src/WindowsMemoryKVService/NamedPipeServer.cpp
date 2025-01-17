#include "NamedPipeServer.h"


NamedPipeServer::NamedPipeServer(SimpleFileLogger& rlogger) : m_logger(rlogger)
{
    m_callback = nullptr;
    m_running = true;
}

void NamedPipeServer::SetCallback(NamedPipeServerCallback callback)
{
    m_callback = callback;
}

bool NamedPipeServer::CallCallback(const std::wstring& message) const
{
    if (m_callback != nullptr) {
        return m_callback(message);
    }
    m_logger.Log(L"No callback set!", 1, true);
    return false;
}

bool NamedPipeServer::CreateNamedPipeObject(HANDLE& hPipe) const
{
    // Create named pipe
    hPipe = CreateNamedPipe(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        512,
        512,
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        DWORD dwError = GetLastError();
        std::wstringstream wss;
        wss << L"Failed to create named pipe. Error code: " << dwError;
        m_logger.Log(wss.str().c_str(), 2, true);
        return false;
    }
    return true;
}

std::wstring NamedPipeServer::StringToWstring(const std::string& str)
{
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (bufferSize == 0) {
        return L"";
    }
    std::wstring result(bufferSize - 1, L'\0');  // The -1 accounts for the null terminator
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], bufferSize);
    return result;
}

bool NamedPipeServer::HandleClientRequest(HANDLE hPipe) const
{
    char buffer[512];
    DWORD bytesRead;

    BOOL success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);

    if (!success || bytesRead == 0) {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_BROKEN_PIPE) {
            m_logger.Log(L"Client disconnected.", 2, true);
            return false;
        }
        std::wstringstream wss;
        wss << L"Error reading from pipe. Error code: " << dwError;
        m_logger.Log(wss.str().c_str(), 2, true);
        return false;
    }

    buffer[bytesRead] = L'\0';  // Null-terminate the string
    std::string byteMessage(buffer);
    std::wstring request = StringToWstring(byteMessage);

    return CallCallback(request);
}

void NamedPipeServer::Serve()
{
    m_logger.Log(L"listen thread starts.", 2, true);

    HANDLE hPipe;
    if (!CreateNamedPipeObject(hPipe))
        return;

    while (m_running)
    {
        m_logger.Log(L"Server is waiting for client connection...");

        auto connected = ConnectNamedPipe(hPipe, NULL);
        if (!connected) {
            std::wstringstream wss;
            DWORD dwError = GetLastError();
            wss << L"Failed to connect to client. Error code: " << dwError << std::endl;
            m_logger.Log(wss.str().c_str(), 2, true);
            CloseHandle(hPipe);
            return;
        }

        m_logger.Log(L"Client connected.", 1, true);

        if (!HandleClientRequest(hPipe))
        {
            m_running = false;
        }

        DisconnectNamedPipe(hPipe);
    }

    CloseHandle(hPipe);
    m_logger.Log(L"listen thread starts.", 2, true);
}
