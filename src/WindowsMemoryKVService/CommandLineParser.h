#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <xstring>

class Config
{
public:
    std::wstring name;              // Mandatory
    int key_length = 64;             // Optional, default to 64
    int value_length = 256;           // Optional, default to 256
    int mmf_count = 100;              // Optional, default to 100
    int block_per_mmf = 1000;          // Optional, default to 1000
    int log_level = 1;              // Optional, default to 1
    int refresh_interval = 10000;       // Optional, default to 10000
};

class ConfigParser
{
public:
    // Function to parse the input string and fill the Config structure
    static bool parseCommandLineArgs(const std::wstring& input, Config& config) {
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
                    throw std::invalid_argument("Missing value for -n");
                }
            }
            else if (token == L"-k") {
                std::wstring value;
                if (wiss >> value) {
                    args[L"-k"] = value;
                }
                else {
                    throw std::invalid_argument("Missing value for -k");
                }
            }
            else if (token == L"-v") {
                std::wstring value;
                if (wiss >> value) {
                    args[L"-v"] = value;
                }
                else {
                    throw std::invalid_argument("Missing value for -v");
                }
            }
            else if (token == L"-m") {
                std::wstring value;
                if (wiss >> value) {
                    args[L"-m"] = value;
                }
                else {
                    throw std::invalid_argument("Missing value for -m");
                }
            }
            else if (token == L"-b") {
                std::wstring value;
                if (wiss >> value) {
                    args[L"-b"] = value;
                }
                else {
                    throw std::invalid_argument("Missing value for -b");
                }
            }
            else if (token == L"-l") {
                std::wstring value;
                if (wiss >> value) {
                    args[L"-l"] = value;
                }
                else {
                    throw std::invalid_argument("Missing value for -l");
                }
            }
            else if (token == L"-i") {
                std::wstring value;
                if (wiss >> value) {
                    args[L"-i"] = value;
                }
                else {
                    throw std::invalid_argument("Missing value for -i");
                }
            }
            else {
                throw std::invalid_argument("Unknown flag: " + std::string(token.begin(), token.end()));
            }
        }

        // Now, map the values from the args to the config structure
        try {
            if (args.find(L"-n") != args.end()) {
                config.name = args[L"-n"];
            }
            else {
                throw std::invalid_argument("Missing mandatory argument: -n");
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
            std::wcerr << L"Invalid argument: " << e.what() << std::endl;
            return false;
        }

        return true;
    }
};