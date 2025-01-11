#include "MemoryKVHostServer.h"

#include <iostream>
#include <Windows.h>
#include <Psapi.h>
#include <sstream>

std::wstring processName = L"WindowsMemoryKVService.exe";
std::wstring executablePath = L"WindowsMemoryKVService.exe";

bool isProcessRunning(const std::wstring& processName) {
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

void startProcess(const std::wstring& executablePath, const std::wstring& arguments) {
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    
    std::wstring command = executablePath + L" " + arguments;
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
    if (!isProcessRunning(processName)) {
        std::wcout << processName << L" is not running. Starting it..." << std::endl;
        std::wstringstream wss;
        wss << " -n " << dbName
            << " -k " << options.MaxKeySize
            << " -v " << options.MaxValueSize
            << " -m " << options.MaxMmfCount
            << " -b " << options.MaxBlocksPerMmf
            << " -i " << refreshInterval;
        startProcess(executablePath, wss.str());
    }
    else {
        std::wcout << processName << L" is already running." << std::endl;
    }
    return true;
}

bool MemoryKVHostServer::Stop()
{
    if (!isProcessRunning(processName))
    {
        std::cout << "process already exit\n";
        return true;
    }

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
    std::cout << "Sent signal to exit the worker thread." << std::endl;
    return true;
}
