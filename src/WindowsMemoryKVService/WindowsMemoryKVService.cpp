#include <codecvt>
#include <iostream>
#include <thread>
#include <string>
#include <cstdlib>  // For std::stoi (string to int conversion)

#include "CommandLineParser.h"
#include "NamedPipeServer.h"
#include "../MemoryKVLib/MemoryKVHostServer.h"

struct command_line_args {
    std::wstring name;
    int key_length{64};
    int value_length{256};
    int mmf_count{100};
    int block_per_mmf{1000};
    int log_level{ 1 };
    int refresh_interval{10000};
};
int refreshInterval = 10000;

std::vector<std::wstring> taskList;
std::unordered_map<std::wstring, std::shared_ptr<MemoryKV>> watchList;
std::mutex taskMutex;
std::condition_variable cv;
std::atomic<bool> running = true;

void WatcherThreadHandler(HANDLE hEvent)
{
    std::wcout << L"watcher thread begins." << std::endl;
    while (true)
    {
        DWORD dwWaitResult = WaitForSingleObject(hEvent, refreshInterval);
        if (dwWaitResult == WAIT_OBJECT_0)
        {
            std::wcout << L"Thread exiting..." << std::endl;
            return;
        }
        else //refresh time out
        {
            std::lock_guard<std::mutex> lock(taskMutex);
            for (const auto& pair : watchList) {
                pair.second->Get(NONE_EXISTED_KEY);
            }
        }
    }
}

bool Shutdown()
{
    HANDLE hEvent = OpenEvent(
        EVENT_ALL_ACCESS,
        FALSE,
        HOST_SERVER_EXIT_EVENT
    );

    if (hEvent == nullptr) {
        std::cerr << "Failed to open event. Error: " << GetLastError() << std::endl;
        return false;
    }
    SetEvent(hEvent);
    CloseHandle(hEvent);
    std::cout << "Sent signal to exit the watcher process." << std::endl;
    return true;
}

std::wstring string_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

bool HandleClientRequest(HANDLE hPipe)
{
    char buffer[512];
    DWORD bytesRead;

    BOOL success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);

    if (!success || bytesRead == 0) {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_BROKEN_PIPE) {
            std::cerr << "Client disconnected." << std::endl;
            return true;
        }
        std::cerr << "Error reading from pipe. Error code: " << dwError << std::endl;
        return true;
    }

    buffer[bytesRead] = L'\0';  // Null-terminate the string
    std::string byteMessage(buffer);
    std::wstring request = string_to_wstring(byteMessage);

    std::wcout << "Received request: " << request << std::endl;

    if (request == L"exit") {
        std::cout << "Shutting down server." << std::endl;
        Shutdown();
        return true;
    }
    else if (request.rfind(L"start ", 0) == 0) {
        // Add task
        std::wstring task = request.substr(6);

        Config config;
        ConfigParser::parseCommandLineArgs(task, config);

        if (watchList.find(config.name) != watchList.end())
        {
            std::wcout << L"dbname " << config.name << L" is already in watch list." << std::endl;
        }
        else
        {
            ConfigOptions options;
            if(config.key_length > 0)
                options.MaxKeySize = config.key_length;
            if (config.value_length > 0)
                options.MaxValueSize = config.value_length;
            if (config.block_per_mmf > 0)
                options.MaxBlocksPerMmf = config.block_per_mmf;
            if (config.mmf_count> 0)
                options.MaxMmfCount = config.mmf_count;
            options.LogLevel = config.log_level; //log level can be zero
            if (config.refresh_interval > 1000)
                refreshInterval = config.refresh_interval;
            const std::shared_ptr<MemoryKV> pKV = std::make_shared<MemoryKV>(L"host_server");
            pKV->Open(config.name.c_str(), options);
            {
                std::lock_guard<std::mutex> lock(taskMutex);
                watchList[config.name] = pKV;
                taskList.push_back(task);
                std::wcout << L"Task added: " << task << std::endl;
            }
        }
        cv.notify_all();
    }
    else if (request.rfind(L"stop ", 0) == 0) {
        std::wstring task = request.substr(5);
        Config config;
        ConfigParser::parseCommandLineArgs(task, config);
        {
            std::lock_guard<std::mutex> lock(taskMutex);
            if (watchList.find(config.name) != watchList.end())
            {
                watchList.erase(config.name);
                std::wcout << L" stop db watcher " << config.name << std::endl;
            }
            if (watchList.empty())
            {
                Shutdown();
                std::wcout << L" the last db watcher exit, stop the process" << std::endl;
                return true;
            }
        }
        cv.notify_all();
    }
    else {
        std::wcout << L"Unknown request: " << request << std::endl;
    }
    return false;
}

bool CreateNamedPipeObject(HANDLE& hPipe)
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
        std::cerr << "Failed to create named pipe. Error code: " << dwError << std::endl;
        if (dwError == ERROR_ACCESS_DENIED) {
            std::cerr << "Access denied. Try running the program as Administrator." << std::endl;
        }
        else if (dwError == ERROR_PIPE_BUSY) {
            std::cerr << "Named pipe is busy. The pipe may already be in use by another process." << std::endl;
        }
        return false;
    }
    return true;
}

void ListenThreadHandler()
{
    std::wcout << L"listen thread starts." << std::endl;

    HANDLE hPipe;
    if (!CreateNamedPipeObject(hPipe))
    {
        std::wcerr << L"create named pipe failed, err code " << GetLastError() << std::endl;
        return;
    }

    while (running)
    {
        std::wcout << L"Server is waiting for client connection..." << std::endl;
        
        auto connected = ConnectNamedPipe(hPipe, NULL);
        if (!connected) {
            DWORD dwError = GetLastError();
            std::wcerr << L"Failed to connect to client. Error code: " << dwError << std::endl;
            CloseHandle(hPipe);
            return;
        }

        std::wcout << L"Client connected." << std::endl;

        if (HandleClientRequest(hPipe))
        {
            running = false;
        }

        DisconnectNamedPipe(hPipe);
    }

    CloseHandle(hPipe);
}

int main()
{
    HANDLE hEvent = CreateEvent(
        nullptr,
        FALSE,
        FALSE,
        HOST_SERVER_EXIT_EVENT
    );
    if (hEvent == nullptr)
    {
        std::wcerr << L"Failed to create event. Error: " << GetLastError() << std::endl;
    }

    std::thread watherThread(WatcherThreadHandler, hEvent);
    std::thread listenThread(ListenThreadHandler);

    listenThread.join();
    watherThread.join();
    CloseHandle(hEvent);
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
