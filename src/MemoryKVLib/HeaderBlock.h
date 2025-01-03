#pragma once
#include <Windows.h>

struct ConfigOptions;

class HeaderBlock
{
private:
    int* pCurrentMMFCount; //starts from 1, 0 means no MMF
    long* pHighestGlobalDbPosition; //starts from 0
    void* pData; //pointer to header MMF start base address

    HANDLE hHeaderMapFile;
    LPVOID pHeaderMapView;
    ConfigOptions& m_options;
    
private:
    void Pin(LPVOID pMapView);
    void ResetHeaderBlock();
    void SetMmfNameAt(int i, const wchar_t* mmfName);
public:
    HeaderBlock(ConfigOptions& options);
    void SetCurrentMMFCount(int count);
    int GetCurrentMMFCount() const;
    void SetHighestGlobalDbPosition(long position);
    long GetHighestGlobalDbPosition() const;
    void Setup();
    void TearDown();
    wchar_t* GetMmfNameAt(int nextMmfSequence);
};
