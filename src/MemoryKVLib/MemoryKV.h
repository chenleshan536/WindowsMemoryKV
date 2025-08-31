#pragma once

#include <string>
#include <unordered_map>
#include <Windows.h>
#include <memory>

#include "ConfigOptions.h"
#include "HeaderBlock.h"
#include "ILogger.h"

struct DataBlock {
    DataBlock(void* pData) { m_pData = pData; }
    void* m_pData;
    //wchar_t* key;
    //wchar_t* value;
    bool IsEmpty() const
    {
        auto key = GetKey();
        return key == nullptr || key[0] == L'\0';
    }

    void SetKey(const wchar_t* str, int max_key_size)
    {
        wcsncpy_s(static_cast<wchar_t*>(m_pData), max_key_size, str, (size_t)max_key_size -1);
    }

    void SetValue(const wchar_t* str, int max_key_size, int max_value_size)
    {
        wcsncpy_s(static_cast<wchar_t*>(m_pData) + max_key_size, max_value_size, str, (size_t)max_value_size-1);
    }

    const wchar_t* GetKey() const
    {
        return static_cast<const wchar_t*>(m_pData);
    }


    const wchar_t* GetValue(int max_key_size) const
    {
        return static_cast<const wchar_t*>(m_pData) + max_key_size;
    }
};


struct KvOomException : std::exception
{
};

struct KvInvalidOptionsException : std::exception
{
};

struct KvMultiInitializationException: std::exception
{};


enum BlockState
{
    Empty,
    Normal,
    Mismatch
};

class MemoryKV {
private:
    std::wstring m_dbName;
    ConfigOptions m_options;
    long m_dataBlockSize{}; // Size of each block (Key + Value)
    HANDLE *hMapFiles{};  // Handle to the memory-mapped file of data block
    LPVOID *pMapViews{};  // Pointer to the memory-mapped view of data block
    HANDLE m_hMutex{};    // Handle to the named mutex
    int m_currentMmfCount{}; //starts from 1, 0 means no data block
    int m_highestKeyPosition{};
    std::unordered_map<std::wstring, long> m_keyPositionMap;
    std::wstring m_clientName;
    std::unique_ptr<ILogger> m_logger;
    HeaderBlock m_pHeaderBlock;

private:

    void InitMutex();
    void InitializeData();
    void InitLocalVars();
    void InitHeaderBlock();
    void InitDataBlock();

    int FindNextAvailableBlock() const;
    void ExpandDataBlock();
    void SyncDataBlock(int dataBlockMmfIndex);
    void SyncDataBlocks();
    void RetrieveGlobalDbIndexByKey(const std::wstring& key, int& dataBlockMmfIndex, int& dataBlockIndex);
    void _FetchAndFindTheBlock(const std::wstring& key, int& dataBlockMmfIndex, int& dataBlockIndex);
    void QueryValueByKey(const std::wstring& key, const wchar_t*& result);
    LPVOID TheCurrentMapView() const;
    void* GetDataBlock(LPVOID pMapView, int i);
    void* GetDataBlock(int dataBlockMmfIndex, int dataBlockIndex) const;
    void RefreshGlobalDbIndex();
    void MarkGlobalDbIndex(const wchar_t* key, long globalDbIndex, bool isKeyFirstAdded);
    void UnmarkGlobalDbIndex(const std::wstring& key, int data_block_mmf_index, int data_block_index, bool isRemovedByMe);
    bool UpdateKeyValue(const std::wstring& key, const std::wstring& value);
    long BuildGlobalDbIndex(int dataBlockmmfIndex, int dataBlockIndex) const;
    void CrackGlobalDbIndex(long globalDbIndex, int& dataBlockMmfIndex, int& dataBlockIndex) const;
    BlockState ValidateBlock(DataBlock& block, const std::wstring& key);
    void RemoveBlockByKey(const std::wstring& key);
    void RemoveData(DataBlock& block) const;
    bool IsInitialized() const;

public:
    __declspec(dllexport) MemoryKV(const wchar_t* clientName, std::unique_ptr<ILogger> logger = nullptr);
    
    __declspec(dllexport) ~MemoryKV();

    __declspec(dllexport) void Open(const wchar_t* dbName, ConfigOptions options = ConfigOptions());    

    __declspec(dllexport) bool Put(const std::wstring& key, const std::wstring& value);

    __declspec(dllexport) const wchar_t* Get(const std::wstring& key);

    __declspec(dllexport) void Remove(const std::wstring& key);
    
};
