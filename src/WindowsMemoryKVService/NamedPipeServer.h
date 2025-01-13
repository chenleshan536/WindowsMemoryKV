#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>

#define PIPE_NAME L"\\\\.\\pipe\\TaskPipe2"

class NamedPipeServer
{
private:
    std::vector<std::string> taskList;
    std::mutex taskMutex;
    std::condition_variable cv;
    bool running = true;

    // Background thread to process tasks
    void backgroundTaskHandler();

    void processClientRequests(HANDLE hPipe);
public:
    int Serve();
};
