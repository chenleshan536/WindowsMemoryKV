#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "NamedPipeClient.h"

#include <codecvt>

std::string wstring_to_string(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

void NamedPipeClient::SendMessage(HANDLE hPipe, const std::wstring& message) const
{
    DWORD bytesWritten;
    BOOL success = WriteFile(
        hPipe,                    // Pipe handle
        message.c_str(),          // Message to send
        message.length(),         // Message length
        &bytesWritten,            // Bytes written
        NULL                       // Overlapped (asynchronous I/O)
    );

    if (!success) {
        DWORD dwError = GetLastError();
        std::cerr << "Error writing to pipe. Error code: " << dwError << std::endl;
    }
    else {
        std::wcout << "Message sent: " << message << std::endl;
    }
}

bool NamedPipeClient::Send(const std::wstring& message) const
{
    HANDLE hPipe;
    DWORD dwWait;

    // Connect to the named pipe
    hPipe = CreateFile(
        PIPE_NAME,            // Pipe name
        GENERIC_WRITE,        // Write access
        0,                    // No sharing
        NULL,                 // Default security attributes
        OPEN_EXISTING,        // Open existing pipe
        0,                    // Default attributes
        NULL                  // No template file
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to connect to pipe." << std::endl;
        return false;
    }

    std::cout << "Client connected to the server." << std::endl;

    DWORD bytesWritten;
    std::string byteMessage = wstring_to_string(message);
    BOOL success = WriteFile(
        hPipe,                    // Pipe handle
        byteMessage.c_str(),          // Message to send
        byteMessage.length(),         // Message length
        &bytesWritten,            // Bytes written
        NULL                       // Overlapped (asynchronous I/O)
    );
    if (!success) {
        DWORD dwError = GetLastError();
        std::cerr << "Error writing to pipe. Error code: " << dwError << std::endl;
        return false;
    }
    else {
        std::wcout << "Message sent: " << message << std::endl;
    }

    CloseHandle(hPipe);
    return true;
}
