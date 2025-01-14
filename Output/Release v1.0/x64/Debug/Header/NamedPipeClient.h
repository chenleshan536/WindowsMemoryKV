#pragma once

#define PIPE_NAME L"\\\\.\\pipe\\TaskPipe2"

class NamedPipeClient
{
private:
    void SendMessage(HANDLE hPipe, const std::wstring& message) const;
public:
    bool Send(const std::wstring& message) const;
};
