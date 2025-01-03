#include <string>
#include <unordered_map>
#include <Windows.h>

#include "ConfigOptions.h"
#include "HeaderBlock.h"
#include "SimpleFileLogger.h"

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
        wcsncpy_s(static_cast<wchar_t*>(m_pData), max_key_size, str, (size_t)max_key_size- 1);
    }

    void SetValue(const wchar_t* str, int max_key_size, int max_value_size)
    {
        wcsncpy_s(static_cast<wchar_t*>(m_pData) + max_key_size, max_value_size, str, (size_t)max_value_size - 1);
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

class MemoryKV {
private:
    long m_dataBlockSize; // Size of each block (Key + Value)
    HANDLE *hMapFiles;  // Handle to the memory-mapped file of data block
    LPVOID *pMapViews;  // Pointer to the memory-mapped view of data block
    HANDLE hMutex;    // Handle to the named mutex
    int m_currentMmfCount; //starts from 1, 0 means no data block
    int m_highestKeyPosition;
    std::unordered_map<std::wstring, long> m_keyPositionMap;
    SimpleFileLogger m_logger;
    ConfigOptions m_options;
    HeaderBlock m_pHeaderBlock;

private:

    void InitMutex();
    void InitializeData();
    void InitLocalVars();
    void InitHeaderBlock();
    void InitDataBlock();

    int FindNextAvailableBlock() const;
    void ExpandDataBlock();
    DataBlock* GetDataBlock(LPVOID pMapView, int i);
    void SyncDataBlock(int dataBlockMmfIndex);
    void SyncDataBlocks();
    void RetrieveGlobalDbIndexByKey(const std::wstring& key, int& dataBlockMmfIndex, int& dataBlockIndex);
    void QueryValueByKey(const std::wstring& key, const wchar_t*& result);
    LPVOID TheCurrentMapView() const;
    long BuildGlobalDbIndex(int dataBlockmmfIndex, int dataBlockIndex) const;
    void* GetDataBlock(int dataBlockMmfIndex, int dataBlockIndex) const;
    void RefreshGlobalDbIndex();
    void MarkGlobalDbIndex(const wchar_t* key, long globalDbIndex, bool isKeyFirstAdded);
    void UpdateKeyValue(const std::wstring& key, const std::wstring& value);


public:
    __declspec(dllexport) MemoryKV(const wchar_t* clientName, ConfigOptions options=ConfigOptions());

    __declspec(dllexport) ~MemoryKV();

    __declspec(dllexport) void Put(const std::wstring& key, const std::wstring& value);
    void CrackGlobalDbIndex(long globalDbIndex, int& dataBlockMmfIndex, int& dataBlockIndex) const;

    __declspec(dllexport) const wchar_t* Get(const std::wstring& key);

    __declspec(dllexport) void Remove(const std::wstring& key);
    
};
