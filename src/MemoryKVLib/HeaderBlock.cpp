#include "HeaderBlock.h"
#include <sstream>
#include <stdexcept>
#include "ConfigOptions.h"
#include "Consts.h"


void HeaderBlock::Pin(LPVOID pMapView)
{
    if (pMapView != nullptr)
    {
        pCurrentMMFCount = static_cast<int*>(pMapView);
        pHighestGlobalDbPosition = reinterpret_cast<long*>(static_cast<char*> (pMapView) + sizeof(int));
        pData = static_cast<char*> (pMapView) + sizeof(int) + sizeof(long);
    }
}

void HeaderBlock::ResetHeaderBlock(std::wstring& dbName)
{
    for (int i = 0; i < m_options.MaxMmfCount; i++)
    {
        std::wstringstream wss;
        wss << L"Global\\MMFDataBlock_"<<dbName<<L"_" << i;
        SetMmfNameAt(i, wss.str().c_str());
    }
    SetCurrentMMFCount(0); //no data block yet
    SetHighestGlobalDbPosition(-1); //next highest position is 0
}

void HeaderBlock::SetMmfNameAt(int i, const wchar_t* mmfName)
{
    wchar_t* pTemp = reinterpret_cast<wchar_t*> (pData) + MAX_MMF_NAME_LENGTH * i;
    wcsncpy_s(pTemp, MAX_MMF_NAME_LENGTH, mmfName, MAX_MMF_NAME_LENGTH - 1);
    pTemp[MAX_MMF_NAME_LENGTH - 1] = L'\0';
}

HeaderBlock::HeaderBlock()
{
    pCurrentMMFCount = nullptr;
    pHighestGlobalDbPosition = nullptr;
    pData = nullptr;
    hHeaderMapFile = nullptr;
    pHeaderMapView = nullptr;
}

void HeaderBlock::SetConfigOptions(ConfigOptions& options)
{
    m_options = options;
}

void HeaderBlock::SetCurrentMMFCount(int count)
{
    *pCurrentMMFCount = count;
}

int HeaderBlock::GetCurrentMMFCount() const
{
    return *pCurrentMMFCount;
}

void HeaderBlock::SetHighestGlobalDbPosition(long position)
{
    *pHighestGlobalDbPosition = position;
}
/*
 * The glboal highest key position (HKP) is used for all clients to sync their data blocks
 * It must be increased everytime a value is added
 * It can be decreased if the last value is removed, but it's dangerous if it's removed at the end of one mmf, then the MMFCount doens't match the global HKP.
 * So an alternative is not to decrease it
 */
long HeaderBlock::GetHighestGlobalDbPosition() const
{
    return *pHighestGlobalDbPosition;
}

void HeaderBlock::Setup(std::wstring& dbName)
{
    std::wstringstream wss;
    wss << L"Global\\MMFHeaderBlock_" << dbName;
    int MmfNameSectionSize = m_options.MaxMmfCount * MAX_MMF_NAME_LENGTH * sizeof(wchar_t);
    int headerSize = MmfNameSectionSize + sizeof(int) + sizeof(long);

    hHeaderMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        headerSize,
        wss.str().c_str());
    if (hHeaderMapFile == nullptr) {
        throw std::runtime_error("Failed to create memory-mapped file.");
    }
    int error = GetLastError();

    pHeaderMapView = MapViewOfFile(
        hHeaderMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        headerSize);
    if (pHeaderMapView == nullptr) {
        CloseHandle(pHeaderMapView);
        throw std::runtime_error("Failed to map view of memory-mapped file.");
    }
    Pin(pHeaderMapView);

    if (error != ERROR_ALREADY_EXISTS) //first time creates
    {
        ResetHeaderBlock(dbName);
    }
}

void HeaderBlock::TearDown()
{
    UnmapViewOfFile(pHeaderMapView);
    CloseHandle(hHeaderMapFile);
    pData = nullptr;
    pCurrentMMFCount = nullptr;
    pHighestGlobalDbPosition = nullptr;
}


wchar_t* HeaderBlock::GetMmfNameAt(int nextMmfSequence)
{
    return reinterpret_cast<wchar_t*>(pData) + nextMmfSequence * MAX_MMF_NAME_LENGTH;
}
