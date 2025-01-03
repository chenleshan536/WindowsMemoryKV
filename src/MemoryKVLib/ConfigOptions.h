#pragma once

struct __declspec(dllexport) ConfigOptions
{
    int MaxKeySize;
    int MaxValueSize;
    int MaxBLocksPerMmf;
    int MaxMmfCount;

    ConfigOptions();
};
