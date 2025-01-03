#include <string>
#include <unordered_map>
#include <Windows.h>
#include "SimpleFileLogger.h"

#define MAX_KEY_SIZE 64
#define MAX_VALUE_SIZE 256
#define MAX_BLOCKS_PER_MMF 1000
#define MAX_MMF_COUNT 100
#define MAX_MMF_NAME_LENGTH 64

struct __declspec(dllexport) ConfigOptions
{
    int MaxKeySize;
    int MaxValueSize;
    int MaxBLocksPerMmf;
    int MaxMmfCount;

    ConfigOptions();
};

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

struct HeaderBlock
{
    int* CurrentMMFCount; //starts from 1, 0 means no MMF
    void* pData;
};

struct KvOomException : std::exception
{
};

class MemoryKV {
private:
    HANDLE hHeaderMapFile;  // Handle to the memory-mapped file of header
    LPVOID pHeaderMapView;  // Pointer to the memory-mapped view of header
    HANDLE *hMapFiles;  // Handle to the memory-mapped file of data block
    LPVOID *pMapViews;  // Pointer to the memory-mapped view of data block
    HANDLE hMutex;    // Handle to the named mutex
    int m_currentMmfCount; //starts from 1, 0 means no data block
    long m_dataBlockSize; // Size of each block (Key + Value)
    HeaderBlock m_pHeaderBlock;
    std::unordered_map<std::wstring, long> m_keyPositionMap;
    SimpleFileLogger m_logger;
    ConfigOptions m_options;

private:
    int FindNextAvailableBlock() const;
    void SetMmfNameAt(int i, const wchar_t* mmfName) const;
    void ResetHeaderBlock() const;
    void InitHeaderBlock();
    void InitMutex();
    wchar_t* GetMmfNameAt(int nextMmfSequence);
    void ExpandDataBlock();
    DataBlock* GetDataBlock(LPVOID pMapView, int i);
    void SyncDataBlock(int dataBlockMmfIndex);
    void InitDataBlock();
    void InitLocalVars();
    void InitializeData();
    void CrackGlobalDbIndex(const std::wstring& key, int& dataBlockMmfIndex, int& dataBlockIndex);
    const wchar_t* GetValueByKey(const std::wstring& key);
    void QueryValueByKey(const std::wstring& key, const wchar_t*& result);
    LPVOID TheCurrentMapView() const;
    long BuildGlobalDbIndex(int dataBlockmmfIndex, int dataBlockIndex) const;
    void* GetDataBlock(int dataBlockMmfIndex, int dataBlockIndex) const;
    void UpdateKeyValue(const std::wstring& key, const std::wstring& value);


public:
    __declspec(dllexport) MemoryKV(const wchar_t* clientName, ConfigOptions options=ConfigOptions());

    __declspec(dllexport) ~MemoryKV();

    __declspec(dllexport) void Put(const std::wstring& key, const std::wstring& value);
    
    __declspec(dllexport) const wchar_t* Get(const std::wstring& key);

    __declspec(dllexport) void Remove(const std::wstring& key);
    
};
