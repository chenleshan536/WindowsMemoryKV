#include <codecvt>
#include <iostream>
#include <thread>
#include <string>
#include <cstdlib>  // For std::stoi (string to int conversion)
#include "../MemoryKVLib/MemoryKVHostServer.h"

struct command_line_args {
    std::wstring name;
    int key_length{};
    int value_length{};
    int mmf_count{};
    int block_per_mmf{};
    int refresh_interval{10000};
};


void print_usage()
{
    std::cout << " Program.exe  -n <name> -k <key_length> -v <value_length> -m <mmf_count> -b <block_per_mmf> -i <refresh_interval>\n"
        << " name is mandatory, the rest are optional\n";
}

bool parse_arguments(int argc, char* argv[], command_line_args& args) {
    if (argc < 3) {  
        print_usage();
        return false;
    }

    try {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "-n" && i + 1 < argc) {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                args.name = converter.from_bytes(argv[++i]);
            }
            else if (arg == "-k" && i + 1 < argc) {
                args.key_length = std::stoi(argv[++i]);
            }
            else if (arg == "-v" && i + 1 < argc) {
                args.value_length = std::stoi(argv[++i]);
            }
            else if (arg == "-m" && i + 1 < argc) {
                args.mmf_count = std::stoi(argv[++i]);
            }
            else if (arg == "-b" && i + 1 < argc) {
                args.block_per_mmf = std::stoi(argv[++i]);
            }
            else if (arg == "-i" && i + 1 < argc) {
                args.refresh_interval = std::stoi(argv[++i]);
            }
            else {
                print_usage();
                return false;
            }
        }
        
        return args.name.length() > 0;
    }
    catch(...)
    {
        print_usage();
        return false;
    }
}

void Run(const wchar_t* host_name, ConfigOptions options, int refreshInterval)
{
    HANDLE hEvent = CreateEvent(
        nullptr,
        FALSE,
        FALSE,
        HOST_SERVER_EXIT_EVENT
    );
    if (hEvent == nullptr)
    {
        std::cerr << "Failed to create event. Error: " << GetLastError() << std::endl;
    }
    MemoryKV kv(host_name, options);
    std::thread worker([&kv, &hEvent, refreshInterval]() {
        while (true)
        {
            DWORD dwWaitResult = WaitForSingleObject(hEvent, refreshInterval);
            if (dwWaitResult == WAIT_OBJECT_0)
            {
                std::wcout << L"Thread exiting..." << std::endl;
                return;
            }
            {
                kv.Get(NONE_EXISTED_KEY);
            }
        }
    });

    worker.join();
    CloseHandle(hEvent);
}

int main(int argc, char* argv[])
{
    command_line_args args;

    // Parse arguments
    if (!parse_arguments(argc, argv, args)) {
        return 1;  // Exit with error if argument parsing fails
    }
    
    ConfigOptions options;
    if (args.key_length > 0)
        options.MaxKeySize = args.key_length;
    if (args.value_length > 0)
        options.MaxValueSize = args.value_length;
    if (args.mmf_count > 0)
        options.MaxMmfCount = args.mmf_count;
    if (args.block_per_mmf > 0)
        options.MaxBlocksPerMmf = args.block_per_mmf;

    int refreshInterval = 10000;
    if (args.refresh_interval > 0)
        refreshInterval = args.refresh_interval;

    Run(args.name.c_str(), options, refreshInterval);
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
