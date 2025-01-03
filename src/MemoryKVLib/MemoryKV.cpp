#include "MemoryKV.h"
#include <stdexcept>
#include <Windows.h>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>

#include "Consts.h"
#include "SyncCall.h"

static ConfigOptions g_DefaultOptions;

ConfigOptions::ConfigOptions()
{
    MaxKeySize = MAX_KEY_SIZE;
    MaxValueSize = MAX_VALUE_SIZE;
    MaxBLocksPerMmf = MAX_BLOCKS_PER_MMF;
    MaxMmfCount = MAX_MMF_COUNT;
}

int MemoryKV::FindNextAvailableBlock() const
{
    for (int i = 0; i < m_options.MaxBLocksPerMmf; ++i) {
        DataBlock block(GetDataBlock(m_pHeaderBlock.GetCurrentMMFCount() - 1, i));
        if (block.IsEmpty()) 
        {
            // Check if the block is empty (key is empty)
            return i;
        }
    }
    return m_options.MaxBLocksPerMmf;  // If no free blocks, return the maxBlocks count to indicate full MMF
}

void MemoryKV::InitHeaderBlock()
{
    m_pHeaderBlock.Setup();
}

void MemoryKV::InitMutex()
{
    // Create a named mutex for synchronization across processes
    hMutex = CreateMutex(nullptr, FALSE, L"Global\\MMFManagerMutex");
    if (hMutex == nullptr) {
        throw std::runtime_error("Failed to create named mutex.");
    }
}

/**
 * \brief create a new data block to hold more data, no matter how
 */
void MemoryKV::ExpandDataBlock()
{
    m_logger.Log(L"expand data block starts");
    if(m_pHeaderBlock.GetCurrentMMFCount() >= m_options.MaxMmfCount)
    {
        m_logger.Log(L"expand data block oom");
        throw KvOomException();
    }

    // next MMF index, starts from 0 because it's C++ array index
    // so current file COUNT is next file INDEX
    int nextMmfSequence = m_pHeaderBlock.GetCurrentMMFCount();

    wchar_t* mmfName = m_pHeaderBlock.GetMmfNameAt(nextMmfSequence);
    
    auto mapSize = m_dataBlockSize * m_options.MaxBLocksPerMmf;

    auto hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        mapSize,
        mmfName);
    if (hMapFile == nullptr) 
    {
        m_logger.Log(L"MMF not created, expand failed.");
        throw std::runtime_error("MMF not created, expand failed.");
    }
    int error = GetLastError();
    if(error == ERROR_ALREADY_EXISTS)
    {
        m_logger.Log(L"MMF already exists, expand failed.");
        throw std::runtime_error("MMF already exists, expand failed.");
    }
    
    auto pMapView = MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        mapSize);
    if (pMapView == nullptr) 
    {
        CloseHandle(hMapFile);
        m_logger.Log(L"Failed to map view of memory-mapped file.");
        throw std::runtime_error("Failed to map view of memory-mapped file.");
    }

    std::memset(pMapView, 0, mapSize);
    hMapFiles[nextMmfSequence] = hMapFile;
    pMapViews[nextMmfSequence] = pMapView;
    m_pHeaderBlock.SetCurrentMMFCount(m_pHeaderBlock.GetCurrentMMFCount() + 1);
    m_currentMmfCount = m_pHeaderBlock.GetCurrentMMFCount();
    std::wstringstream ss;
    ss << L"expand data block finished, current mmf count = " << m_currentMmfCount;
    m_logger.Log(ss.str().data());
}

DataBlock* MemoryKV::GetDataBlock(LPVOID pMapView, int i)
{
    return reinterpret_cast<DataBlock*>(static_cast<char*>(pMapView) + i * m_dataBlockSize);
}

void MemoryKV::SyncDataBlock(int dataBlockMmfIndex)
{
    std::wstringstream ss;
    ss << L"sync data block starts, mmf index = " << dataBlockMmfIndex;
    m_logger.Log(ss.str().data());
    wchar_t* mmfName = m_pHeaderBlock.GetMmfNameAt(dataBlockMmfIndex);
    auto mapSize = m_dataBlockSize * m_options.MaxBLocksPerMmf;

    auto hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        mapSize,
        mmfName);
    if (hMapFile == nullptr)
    {
        m_logger.Log(L"MMF not created, sync failed.");
        throw std::runtime_error("MMF not created, sync failed.");
    }
    int error = GetLastError();
    if (error != ERROR_ALREADY_EXISTS)
    {
        m_logger.Log(L"MMF doesn't exists, sync failed.");
        throw std::runtime_error("MMF doesn't exists, sync failed.");
    }

    auto pMapView = MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        mapSize);
    if (pMapView == nullptr)
    {
        CloseHandle(hMapFile);
        m_logger.Log(L"Failed to map view of memory-mapped file.");
        throw std::runtime_error("Failed to map view of memory-mapped file.");
    }

    for (int i = 0; i < m_options.MaxBLocksPerMmf; ++i)
    {
        DataBlock block(GetDataBlock(pMapView, i));
        if (!block.IsEmpty())
        {
            long globalDbIndex = BuildGlobalDbIndex(dataBlockMmfIndex, i);
            m_keyPositionMap[block.GetKey()] = globalDbIndex;
            m_highestKeyPosition = globalDbIndex;
        }
    }

    hMapFiles[dataBlockMmfIndex] = hMapFile;
    pMapViews[dataBlockMmfIndex] = pMapView;

    ss.str(std::wstring());
    ss << L"sync data block finished, mmf index = " << dataBlockMmfIndex;
    m_logger.Log(ss.str().data());
}

void MemoryKV::SyncDataBlocks()
{
    while ( m_currentMmfCount < m_pHeaderBlock.GetCurrentMMFCount())
    {
        SyncDataBlock(m_currentMmfCount);
        m_currentMmfCount++;
    }
}

void MemoryKV::InitDataBlock()
{
    if (m_pHeaderBlock.GetCurrentMMFCount() == 0) // to be deleted later
        ExpandDataBlock();
    else
    {
        SyncDataBlocks();
    }
}

void MemoryKV::InitLocalVars()
{
    m_dataBlockSize = (m_options.MaxKeySize + m_options.MaxValueSize) * sizeof(wchar_t);
    m_currentMmfCount = 0;
    m_highestKeyPosition = 0;
    hMapFiles = new HANDLE[m_options.MaxMmfCount];
    pMapViews = new LPVOID[m_options.MaxMmfCount];

    for (int i = 0; i < m_options.MaxMmfCount; i++)
    {
        hMapFiles[i] = nullptr;
        pMapViews[i] = nullptr;
    }
    m_keyPositionMap.clear();
}

void MemoryKV::InitializeData()
{
    std::wstringstream ss;
    ss << L"initialization starts. max_key_size=" << m_options.MaxKeySize
        << L",max_value_size=" << m_options.MaxValueSize
        << L",max_data_block_count=" << m_options.MaxBLocksPerMmf
        << L",max_mmf_count=" << m_options.MaxMmfCount
        << L",current_mmf_count = " << m_currentMmfCount;
    m_logger.Log(ss.str().data());
    InitLocalVars();
    InitHeaderBlock();
    InitDataBlock();

    m_logger.Log(L"initialization done.");
}

MemoryKV::MemoryKV(const wchar_t* clientName, ConfigOptions options):
m_logger(clientName),
m_options(options),
m_pHeaderBlock(m_options)
{
    InitMutex();
    SYNC_CALL(InitializeData())
}

MemoryKV::~MemoryKV()
{
    m_pHeaderBlock.TearDown();
    for (int i = 0; i < m_options.MaxMmfCount; i++)
    {
        if(pMapViews[i]!=nullptr)
            UnmapViewOfFile(pMapViews[i]);
        if(hMapFiles[i]!= nullptr)
            CloseHandle(hMapFiles[i]);
    }
    delete[] pMapViews;
    delete[] hMapFiles;
    CloseHandle(hMutex);  // Clean up the mutex handle
}

LPVOID MemoryKV::TheCurrentMapView() const
{
    return pMapViews[m_pHeaderBlock.GetCurrentMMFCount() - 1];
}

long MemoryKV::BuildGlobalDbIndex(int dataBlockmmfIndex, int dataBlockIndex) const
{
    return dataBlockmmfIndex * m_options.MaxBLocksPerMmf + dataBlockIndex;
}

void* MemoryKV::GetDataBlock(int dataBlockMmfIndex, int dataBlockIndex) const
{
    return static_cast<char*>(pMapViews[dataBlockMmfIndex]) + dataBlockIndex * m_dataBlockSize;
}

void MemoryKV::RefreshGlobalDbIndex()
{
    m_logger.Log(L"refresh global db index begin");
    int mmfIndex = 0;
    int blockIndex = 0;
    CrackGlobalDbIndex(m_highestKeyPosition, mmfIndex, blockIndex);

    if(mmfIndex != m_currentMmfCount-1)
    {
        m_logger.Log(L"global db index inconsistent, warning!");
    }
    
    for (int i = blockIndex+1; i< m_options.MaxBLocksPerMmf; i++)
    {
        DataBlock block(GetDataBlock(mmfIndex, i));
        if(!block.IsEmpty())
        {
            long newGlobalDbIndex = BuildGlobalDbIndex(mmfIndex, i);
            m_keyPositionMap[block.GetKey()] = newGlobalDbIndex;
            m_highestKeyPosition = newGlobalDbIndex;
            std::wstringstream wss;
            wss << L"refresh mmf_index=" << m_currentMmfCount - 1 << ",dataBlockIndex=" << i;
            m_logger.Log(wss.str().c_str());
        }
    }

    SyncDataBlocks();

    m_logger.Log(L"refresh global db index end");
}

void MemoryKV::UpdateKeyValue(const std::wstring& key, const std::wstring& value)
{
    std::wstringstream ss;
    ss << L"Put key=" << key.c_str() << L",value=" << value.c_str();
    m_logger.Log(ss.str().data());

    if (static_cast<int>(key.size()) >= m_options.MaxKeySize || 
        static_cast<int>(value.size()) >= m_options.MaxValueSize) 
    {
        throw std::invalid_argument("Key or value is too large.");
    }
    
    int dataBlockMmfIndex;
    int dataBlockIndex;
    RetrieveGlobalDbIndexByKey(key, dataBlockMmfIndex, dataBlockIndex);
    
    if (dataBlockMmfIndex == -1 || dataBlockIndex == -1) // not exist before, refresh in case somebody added it recently
    {
        RefreshGlobalDbIndex();
        RetrieveGlobalDbIndexByKey(key, dataBlockMmfIndex, dataBlockIndex);
    }
    if (dataBlockMmfIndex == -1 || dataBlockIndex == -1) // not exist till now, create new
    {
        dataBlockIndex = FindNextAvailableBlock();
        if (dataBlockIndex >= m_options.MaxBLocksPerMmf) {
            ExpandDataBlock();
            dataBlockIndex = FindNextAvailableBlock();
        }
        dataBlockMmfIndex = m_pHeaderBlock.GetCurrentMMFCount() - 1;
        long globalDbIndex = BuildGlobalDbIndex(dataBlockMmfIndex, dataBlockIndex);
        m_keyPositionMap[key] = globalDbIndex;
        m_highestKeyPosition = globalDbIndex;
        ss.str(std::wstring());
        ss << L"find new slot. mmf index=" << dataBlockMmfIndex << L",data block index=" << dataBlockIndex;
        m_logger.Log(ss.str().data());
    }
    else //key exist before, verify its key-position is correct
    {
        ss.str(std::wstring());
        ss << L"find existing slot. mmf index=" << dataBlockMmfIndex << L",data block index=" << dataBlockIndex;
        m_logger.Log(ss.str().data());

        DataBlock block(GetDataBlock(dataBlockMmfIndex, dataBlockIndex));
        if(block.IsEmpty())
        {
            m_logger.Log(L"the block is empty");
            throw std::runtime_error("the block is empty");
        }
        if (std::wcsncmp(block.GetKey(), key.c_str(), m_options.MaxKeySize) != 0)
        {
            ss.str(std::wstring());
            ss << L"mismatched key and position. the key in block is " << block.GetKey();
            m_logger.Log(ss.str().data());
            throw std::runtime_error("key and position does not match");
        }
    }

    DataBlock block = GetDataBlock(dataBlockMmfIndex, dataBlockIndex);
    block.SetKey(key.c_str(), m_options.MaxKeySize);
    block.SetValue(value.c_str(), m_options.MaxKeySize, m_options.MaxValueSize);
    /*wcsncpy_s(block->key, m_options.MaxKeySize, key.c_str(), (size_t) m_options.MaxKeySize-1);
    wcsncpy_s(block->value, m_options.MaxKeySize, value.c_str(), (size_t) m_options.MaxValueSize-1);*/
    m_logger.Log(L"put value successfully");
}

void MemoryKV::Put(const std::wstring& key, const std::wstring& value)
{
    SYNC_CALL(UpdateKeyValue(key, value))
}

void MemoryKV::CrackGlobalDbIndex(long globalDbIndex, int& dataBlockMmfIndex, int& dataBlockIndex) const
{
    dataBlockMmfIndex = static_cast<int>(globalDbIndex / m_options.MaxBLocksPerMmf);
    dataBlockIndex = static_cast<int>(globalDbIndex % m_options.MaxBLocksPerMmf);
}

void MemoryKV::RetrieveGlobalDbIndexByKey(const std::wstring& key, int& dataBlockMmfIndex, int& dataBlockIndex)
{
    dataBlockMmfIndex = -1;
    dataBlockIndex = -1;

    if (m_keyPositionMap.find(key) != m_keyPositionMap.end())
    {
        long globalDbIndex = m_keyPositionMap[key];
        CrackGlobalDbIndex(globalDbIndex, dataBlockMmfIndex, dataBlockIndex);
    }
}


void MemoryKV::QueryValueByKey(const std::wstring& key, const wchar_t*& result)
{
    std::wstringstream ss;
    ss << L"Get key=" << key.c_str();

    int dataBlockMmfIndex;
    int dataBlockIndex;
    RetrieveGlobalDbIndexByKey(key, dataBlockMmfIndex, dataBlockIndex);
    if (dataBlockMmfIndex == -1 || dataBlockIndex == -1) // not exist before, refresh in case somebody added it recently
    {
        RefreshGlobalDbIndex();
        RetrieveGlobalDbIndexByKey(key, dataBlockMmfIndex, dataBlockIndex);
    }
    if(dataBlockMmfIndex == -1 || dataBlockIndex == -1) // not found
    {
        result = L"";
        ss << L". not found";
        m_logger.Log(ss.str().data());
    }
    else 
    {
        ss << L". find the slot: mmf index=" << dataBlockMmfIndex << L",data block index=" << dataBlockIndex;
        DataBlock block(GetDataBlock(dataBlockMmfIndex, dataBlockIndex));

        if (block.IsEmpty())
        {
            m_logger.Log(L"the block is empty");
            throw std::runtime_error("the block is empty");
        }
        if (std::wcsncmp(block.GetKey(), key.c_str(), m_options.MaxKeySize) != 0)
        {
            ss.str(std::wstring());
            ss << L". mismatched key and position. the key in block is " << block.GetKey();
            m_logger.Log(ss.str().data());
            throw std::runtime_error("key and position does not match");
        }

        ss << L",value=" << block.GetValue(m_options.MaxKeySize);
        m_logger.Log(ss.str().data());
        result = block.GetValue(m_options.MaxKeySize);
    }
}

const wchar_t* MemoryKV::Get(const std::wstring& key)
{
    const wchar_t* result;
    SYNC_CALL(QueryValueByKey(key, result))
    return result;
}

void MemoryKV::Remove(const std::wstring& key)
{
    // To-Do
}
