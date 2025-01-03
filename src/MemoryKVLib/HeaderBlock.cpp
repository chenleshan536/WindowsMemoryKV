#include "HeaderBlock.h"
#include <sstream>
#include <stdexcept>
#include "ConfigOptions.h"
#include "Consts.h"


void HeaderBlock::Pin(LPVOID pMapView)
{
    if (pMapView != nullptr)
    {
        CurrentMMFCount = static_cast<int*>(pMapView);
        pData = static_cast<char*> (pMapView) + sizeof(int);
    }
}

void HeaderBlock::ResetHeaderBlock()
{
    for (int i = 0; i < m_options.MaxMmfCount; i++)
    {
        std::wstringstream wss;
        wss << L"Global\\MyMemoryKVDataBlock_" << i;
        SetMmfNameAt(i, wss.str().c_str());
    }
    SetCurrentMMFCount(0); //no data block yet
}

void HeaderBlock::SetMmfNameAt(int i, const wchar_t* mmfName)
{
    wchar_t* pTemp = reinterpret_cast<wchar_t*> (pData) + MAX_MMF_NAME_LENGTH * i;
    wcsncpy_s(pTemp, MAX_MMF_NAME_LENGTH, mmfName, MAX_MMF_NAME_LENGTH - 1);
    pTemp[MAX_MMF_NAME_LENGTH - 1] = L'\0';
}

HeaderBlock::HeaderBlock(ConfigOptions& options): m_options(options)
{
    CurrentMMFCount = nullptr;
    pData = nullptr;
    hHeaderMapFile = nullptr;
    pHeaderMapView = nullptr;
}

    void HeaderBlock::SetCurrentMMFCount(int count)
    {
        *CurrentMMFCount = count;
    }

    int HeaderBlock::GetCurrentMMFCount() const
    {
        return *CurrentMMFCount;
    }

    void HeaderBlock::Setup()
    {
        const wchar_t* HEADER_MMF_NAME = L"Global\\MyMemoryKVHeaderBlock";
        int MmfCountSectionSize = sizeof(int);
        int MmfNameSectionSize = m_options.MaxMmfCount * MAX_MMF_NAME_LENGTH * sizeof(wchar_t);
        int headerSize = MmfCountSectionSize + MmfNameSectionSize;

        hHeaderMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            headerSize,
            HEADER_MMF_NAME);
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
            ResetHeaderBlock();
        }
    }

    void HeaderBlock::TearDown()
    {
        UnmapViewOfFile(pHeaderMapView);
        CloseHandle(hHeaderMapFile);
        pData = nullptr;
        CurrentMMFCount = nullptr;
    }


    wchar_t* HeaderBlock::GetMmfNameAt(int nextMmfSequence)
    {
        return reinterpret_cast<wchar_t*>(pData) + nextMmfSequence * MAX_MMF_NAME_LENGTH;
    }
