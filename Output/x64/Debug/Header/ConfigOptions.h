#pragma once

struct __declspec(dllexport) ConfigOptions
{
    int MaxKeySize;
    int MaxValueSize;
    int MaxBlocksPerMmf;
    int MaxMmfCount;
    ConfigOptions();
};
