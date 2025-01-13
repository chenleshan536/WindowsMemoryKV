#pragma once
#include <condition_variable>
#include <mutex>
#include "MemoryKV.h"

#define NONE_EXISTED_KEY L"{9F4EEC75-2F8B-415F-84D5-85C8202156AB}"
#define HOST_SERVER_EXIT_EVENT L"Host_Service_Exit_Event"


class MemoryKVHostServer
{
public:
    __declspec(dllexport) static bool Run(const wchar_t* dbName, ConfigOptions options=ConfigOptions(), int refreshInterval=10000);
    __declspec(dllexport) static bool Stop();
    __declspec(dllexport) static bool Stop(const wchar_t* dbName);
};

