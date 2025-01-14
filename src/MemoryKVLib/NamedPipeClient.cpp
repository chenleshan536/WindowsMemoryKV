#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "NamedPipeClient.h"

#include <codecvt>

std::string wstring_to_string(const std::wstring& wstr) {
    // Calculate the required buffer size
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    
    if (bufferSize == 0) {
        // Handle conversion failure (optional)
        return "";
    }

    // Create a buffer to hold the result
    std::string result(bufferSize - 1, '\0');  // The -1 accounts for the null terminator

    // Perform the actual conversion
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], bufferSize, nullptr, nullptr);

    return result;
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
