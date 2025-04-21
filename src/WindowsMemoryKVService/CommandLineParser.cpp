#include "CommandLineParser.h"
#include "../MemoryKVLib/SimpleFileLogger.h"

bool ConfigParser::ParseCommandLineArgs(const std::wstring& input, Config& config, SimpleFileLogger& logger)
{
    std::wistringstream wiss(input);
    std::unordered_map<std::wstring, std::wstring> args;

    std::wstring token;
    while (wiss >> token) {
        if (token == L"-n") {
            std::wstring value;
            if (wiss >> value) {
                args[L"-n"] = value;
            }
            else {
                logger.Log(L"Missing value for -n",1, true);
            }
        }
        else if (token == L"-k") {
            std::wstring value;
            if (wiss >> value) {
                args[L"-k"] = value;
            }
            else {
                logger.Log(L"Missing value for -k");
            }
        }
        else if (token == L"-v") {
            std::wstring value;
            if (wiss >> value) {
                args[L"-v"] = value;
            }
            else {
                logger.Log(L"Missing value for -v");
            }
        }
        else if (token == L"-m") {
            std::wstring value;
            if (wiss >> value) {
                args[L"-m"] = value;
            }
            else {
                logger.Log(L"Missing value for -m");
            }
        }
        else if (token == L"-b") {
            std::wstring value;
            if (wiss >> value) {
                args[L"-b"] = value;
            }
            else {
                logger.Log(L"Missing value for -b");
            }
        }
        else if (token == L"-l") {
            std::wstring value;
            if (wiss >> value) {
                args[L"-l"] = value;
            }
            else {
                logger.Log(L"Missing value for -l");
            }
        }
        else if (token == L"-i") {
            std::wstring value;
            if (wiss >> value) {
                args[L"-i"] = value;
            }
            else {
                logger.Log(L"Missing value for -i");
            }
        }
        else {
            std::wstringstream wss;
            wss << L"Unknown flag: " <<token;
            logger.Log(wss.str().c_str());
        }
    }

    // Now, map the values from the args to the config structure
    try {
        if (args.find(L"-n") != args.end()) {
            config.name = args[L"-n"];
        }
        else {
            logger.Log(L"Missing mandatory argument: -n");
        }

        // For optional arguments, we assign values only if they exist in the args map
        if (args.find(L"-k") != args.end()) {
            config.key_length = std::stoi(std::string(args[L"-k"].begin(), args[L"-k"].end()));
        }
        if (args.find(L"-v") != args.end()) {
            config.value_length = std::stoi(std::string(args[L"-v"].begin(), args[L"-v"].end()));
        }
        if (args.find(L"-m") != args.end()) {
            config.mmf_count = std::stoi(std::string(args[L"-m"].begin(), args[L"-m"].end()));
        }
        if (args.find(L"-b") != args.end()) {
            config.block_per_mmf = std::stoi(std::string(args[L"-b"].begin(), args[L"-b"].end()));
        }
        if (args.find(L"-l") != args.end()) {
            config.log_level = std::stoi(std::string(args[L"-l"].begin(), args[L"-l"].end()));
        }
        if (args.find(L"-i") != args.end()) {
            config.refresh_interval = std::stoi(std::string(args[L"-i"].begin(), args[L"-i"].end()));
        }
    }
    catch (const std::invalid_argument& e) {
        std::wstringstream wss;
        wss << L"Invalid argument: " << e.what();
        logger.Log(wss.str().c_str());
        return false;
    }

    return true;
}
