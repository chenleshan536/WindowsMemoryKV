#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <xstring>

class SimpleFileLogger;

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
    static bool ParseCommandLineArgs(const std::wstring& input, Config& config, SimpleFileLogger& logger);
};