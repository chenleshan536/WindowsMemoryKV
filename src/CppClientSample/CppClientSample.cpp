// CppClientSample.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <sstream>

#include "../MemoryKVLib/MemoryKV.h"
#include "../MemoryKVLib/MemoryKVHostServer.h"

bool ParseInput(const std::wstring& input, std::wstring& mode, std::wstring& valuePrefix, int& count, int& startIndex)
{
    size_t start = 0, end;
    try {
        // Loop through the string and split by space
        if ((end = input.find(' ', start)) != std::wstring::npos)
        {
            mode = input.substr(start, end - start);
            start = end + 1;
        }
        else
        {
            mode = input;
            return true;
        }

        if ((end = input.find(' ', start)) != std::wstring::npos)
        {
            valuePrefix = input.substr(start, end - start);
            start = end + 1;
        }

        if ((end = input.find(' ', start)) != std::wstring::npos)
        {
            auto temp = input.substr(start, end - start);
            count = std::stoi(temp);
            start = end + 1;
        }

        {
            auto temp = input.substr(start);
            startIndex = std::stoi(temp);
        }
    }
    catch (...)
    {
        return false;
    }
    return true;
}

void TestPut(MemoryKV& kv, const std::wstring& valuePrefix, int count, int start_index)
{
    for (int i = 0; i < count; i++)
    {
        std::wstringstream ss;
        ss << L"key" << (i + start_index);
        auto key = ss.str();
        ss.str(std::wstring());
        ss << valuePrefix << (i + start_index);
        auto value = ss.str();
        kv.Put(key, value);
        Sleep(100);
    }
    std::cout << "test put " << count << " " << start_index << " done.\n";
}

void TestGet(MemoryKV& kv, const std::wstring& valuePrefix, int count, int start_index)
{
    for (int i = 0; i < count; i++)
    {
        std::wstringstream ss;
        ss << L"key" << (i + start_index);
        auto key = ss.str();
        auto real_value = kv.Get(key);
        ss.str(std::wstring());
        ss << valuePrefix << (i + start_index);
        auto expected_value = ss.str();
        if(real_value!=expected_value)
        {
            std::wcout << L"key=" << key << L" value changed. expected=" << expected_value << ",real=" << real_value << std::endl;
        }
        Sleep(100);
    }
    std::cout << "test get " << count << " " << start_index << " done.\n";
}

void StartHostService()
{
    MemoryKVHostServer::Run(L"host_server");
}

void StopHostService()
{
    MemoryKVHostServer::Stop();
}

int main()
{
    ConfigOptions option;
    option.MaxMmfCount = 100;

    MemoryKV kv(L"cppclient", option);
    std::wcout << L"start testing.\n";
    while (true)
    {
        std::wcout << L"Usages:\n"
            << L"\'starthost\', start host service \n"
            << L"\'stophost\', stop host service \n"
            << L"\'put value 100 1\', put 100 values from the given key sequence 1, using predefined key (key1, key2, ...) and value (value1, value2, ...) \n"
            << L"\'get xyz 100 5\', get 100 values from the given key sequence 5, using predefined key (key5, key6, ...) and check value against (xyz5, xyz6, ...)\n"
            << L"\'exit\', to exit testing\n";
        std::wstring input;
        std::getline(std::wcin, input);
        std::wstring mode;
        std::wstring valuePrefix;
        int count;
        int startIndex;
        if(!ParseInput(input, mode, valuePrefix, count, startIndex))
            continue;

        if(mode ==L"put")
        {
            TestPut(kv, valuePrefix, count, startIndex);
        }
        else if(mode==L"get")
        {
            TestGet(kv, valuePrefix, count, startIndex);
        }
        if (mode == L"starthost")
        {
            StartHostService();
        }
        if (mode == L"stophost")
        {
            StopHostService();
        }
        else if (mode==L"exit")
        {
            std::wcout << L"exit testing\n";
            return 1;
        }
        else
        {
            std::wcout << L"wrong input\n";
        }
    }
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
