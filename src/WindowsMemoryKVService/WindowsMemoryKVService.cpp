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
std::unordered_map<std::wstring, std::shared_ptr<MemoryKV>> watchList;
std::mutex taskMutex;
SimpleFileLogger logger(L"kvhostserver");

void WatcherThreadHandler(HANDLE hEvent)
{
    logger.Log(L"watcher thread begins.", 1, true);
    while (true)
    {
        DWORD dwWaitResult = WaitForSingleObject(hEvent, refreshInterval);
        if (dwWaitResult == WAIT_OBJECT_0)
        {
            logger.Log(L"Thread exiting...", 1, true);
            return;
        }
        else //refresh time out
        {
            logger.Log(L"refresh db begins", 1, true);
            std::lock_guard<std::mutex> lock(taskMutex);
            for (const auto& pair : watchList) {
                std::wstringstream wss;
                wss << L"refresh db " << pair.first;
                logger.Log(wss.str().c_str(), 1, true);
                pair.second->Get(NONE_EXISTED_KEY);
            }
            logger.Log(L"refresh db ends", 1, true);
        }
    }
}

/**
 * \brief 
 * \return continue the service or not, always false
 */
bool Shutdown()
{
    logger.Log(L"Shutting down server.",1,true);
    HANDLE hEvent = OpenEvent(
        EVENT_ALL_ACCESS,
        FALSE,
        HOST_SERVER_EXIT_EVENT
    );

    if (hEvent == nullptr) {
        std::wstringstream wss;
        wss << L"Failed to open event. Error: " << GetLastError();
        logger.Log(wss.str().c_str(), 2, true);
    }
    else 
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
        logger.Log(L"Sent signal to exit the watcher process.");
    }
    return false;
}

/**
 * \brief 
 * \param request 
 * \return continue the service or not, always true
 */
bool HandleStartRequest(const std::wstring& request)
{
    std::wstring task = request.substr(6);

    Config config;
    ConfigParser::ParseCommandLineArgs(task, config, logger);

    if (watchList.find(config.name) != watchList.end())
    {
        std::wstringstream wss;
        wss << L"dbname " << config.name << L" is already in watch list.";
        logger.Log(wss.str().c_str(), 1, true);
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
            std::wstringstream wss;
            wss << L"Task added: " << task;
            logger.Log(wss.str().c_str(), 1, true);
        }
    }
    return true;
}

/**
 * \brief 
 * \param request 
 * \return continue the service or not
 */
bool HandleStopRequest(std::wstring request)
{
    std::wstring task = request.substr(5);
    Config config;
    ConfigParser::ParseCommandLineArgs(task, config, logger);
    {
        std::lock_guard<std::mutex> lock(taskMutex);
        if (watchList.find(config.name) != watchList.end())
        {
            watchList.erase(config.name);
            std::wstringstream wss;
            wss << L" stop db watcher " << config.name;
            logger.Log(wss.str().c_str(), 1, true);
        }
        if (watchList.empty())
        {
            Shutdown();
            logger.Log(L" the last db watcher exit, stop the process", 1, true);
            return false;
        }
    }
    return true;
}


/**
 * \brief
 * \param request
 * \return continue the service or not
 */
bool HandleRequest(const std::wstring& request)
{
    std::wstringstream wss;
    wss << L"Received request: " << request;
    logger.Log(wss.str().c_str(), 1, true);

    if (request == L"exit") {
        return Shutdown();
    }

    if (request.rfind(L"start ", 0) == 0) {
        return HandleStartRequest(request);
    }

    if (request.rfind(L"stop ", 0) == 0) {
        return HandleStopRequest(request);
    }

    logger.Log(L"Unknown request.");
    return true;
}

void ListenThreadHandler()
{
    NamedPipeServer server(logger);
    NamedPipeServerCallback callBack = HandleRequest;
    server.SetCallback(callBack);
    server.Serve();
}

int main()
{
    logger.Log(L"kv host server starts", 1, true);
    HANDLE hEvent = CreateEvent(
        nullptr,
        FALSE,
        FALSE,
        HOST_SERVER_EXIT_EVENT
    );
    if (hEvent == nullptr)
    {
        std::wstringstream wss;
        wss << L"Failed to create event. Error: " << GetLastError();
        logger.Log(wss.str().c_str());
        return 1;
    }

    std::thread watherThread(WatcherThreadHandler, hEvent);
    std::thread listenThread(ListenThreadHandler);
    
    listenThread.join();
    watherThread.join();
    CloseHandle(hEvent);
    logger.Log(L"kv host server exits", 1, true);
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
