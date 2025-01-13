#include "MemoryKVHostServer.h"

#include <iostream>
#include <Windows.h>
#include <Psapi.h>
#include <sstream>

#include "NamedPipeClient.h"

std::wstring processName = L"WindowsMemoryKVService.exe";
std::wstring executablePath = L"WindowsMemoryKVService.exe";

bool IsProcessRunning(const std::wstring& processName) {
    // Get the list of all process IDs
    DWORD processes[1024], cbNeeded, processCount;
    if (!EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
        std::cerr << "Failed to enumerate processes." << std::endl;
        return false;
    }

    processCount = cbNeeded / sizeof(DWORD);

    // Iterate through all the processes
    for (unsigned int i = 0; i < processCount; ++i) {
        if (processes[i] == 0) {
            continue;
        }

        // Open the process
        DWORD processID = processes[i];
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
        if (hProcess) {
            TCHAR processNameBuffer[MAX_PATH];
            if (GetModuleBaseName(hProcess, NULL, processNameBuffer, sizeof(processNameBuffer) / sizeof(TCHAR))) {
                if (_wcsicmp(processNameBuffer, processName.c_str()) == 0) {
                    CloseHandle(hProcess);
                    return true;  // Process is running
                }
            }
            CloseHandle(hProcess);
        }
    }
    return false;  // Process not found
}

void StartProcess(const std::wstring& executablePath) {
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    
    std::wstring command = executablePath;
    if (!CreateProcess(NULL, &command[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to start the process." << std::endl;
    }
    else {
        std::cout << "Process started successfully." << std::endl;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}


bool MemoryKVHostServer::Run(const wchar_t* dbName, ConfigOptions options, int refreshInterval) {
    if (!IsProcessRunning(processName)) {
        std::wcout << processName << L" is not running. Starting it..." << std::endl;
        StartProcess(executablePath);
    }
    else {
        std::wcout << processName << L" is already running. add watcher for db" << dbName << std::endl;
    }

    std::wstringstream wss;
    wss << L"start -n " << dbName
        << L" -k " << options.MaxKeySize
        << L" -v " << options.MaxValueSize
        << L" -m " << options.MaxMmfCount
        << L" -b " << options.MaxBlocksPerMmf
        << L" -l " << options.LogLevel
        << L" -i " << refreshInterval;

    NamedPipeClient client;
    return client.Send(wss.str());
}

bool MemoryKVHostServer::Stop()
{
    if (!IsProcessRunning(processName))
    {
        std::wcout << L"process already exit\n";
        return true;
    }

    NamedPipeClient client;
    return client.Send(L"exit");
}

bool MemoryKVHostServer::Stop(const wchar_t* dbName)
{
    if (!IsProcessRunning(processName))
    {
        std::wcout << L"process already exit\n";
        return true;
    }

    std::wstringstream wss;
    wss << L"stop -n" << dbName;
    NamedPipeClient client;
    std::wcout << L"Sent signal to exit the watcher for db - " << dbName << std::endl;
    return client.Send(wss.str());
}
