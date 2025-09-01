#include "MemoryKV.h"
#include <stdexcept>
#include <Windows.h>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>

#include "Consts.h"
#include "SyncCall.h"
#include "SimpleFileLogger.h"

ConfigOptions::ConfigOptions()
{
    MaxKeySize = MAX_KEY_SIZE;
    MaxValueSize = MAX_VALUE_SIZE;
    MaxBlocksPerMmf = MAX_BLOCKS_PER_MMF;
    MaxMmfCount = MAX_MMF_COUNT;
    LogLevel = 1;
}

bool ConfigOptions::Validate() const
{
    return (MaxKeySize > 0 
        && MaxValueSize > 0
        && MaxBlocksPerMmf > 0
        && MaxMmfCount > 0);
}

/// <summary>
/// only find it from the last active blocks of the last mmf, ignore those data blocks that have been removed
/// reusing those removed data blocks is very complex, which means we need to let the m_highestKeyPosition
/// jump back and handle all key indexes re-sync again. Let's assume remove is not a heavy operation so there won't
/// be a lot of space waste
/// </summary>
/// <returns></returns>
int MemoryKV::FindNextAvailableBlock() const
{
    int mmfIndex;
    int blockIndex;
    CrackGlobalDbIndex(m_highestKeyPosition, mmfIndex, blockIndex);

    for (int i = blockIndex+1; i < m_options.MaxBlocksPerMmf; ++i) {
        DataBlock block(GetDataBlock(mmfIndex, i));
        if (block.IsEmpty())
        {
            return i;
        }
    }
    return m_options.MaxBlocksPerMmf;  // If no free blocks, return the maxBlocks count to indicate full MMF
}

void MemoryKV::InitHeaderBlock()
{
    m_pHeaderBlock.Setup(m_dbName);
}

void MemoryKV::InitMutex()
{
    std::wstringstream wss;
    wss << L"Global\\MMFMutex_" << m_dbName;
    m_hMutex = CreateMutex(nullptr, FALSE, wss.str().c_str());
    if (m_hMutex == nullptr) {
        m_logger->Log(L"Failed to create named mutex.");
        throw std::runtime_error("Failed to create named mutex.");
    }
}

/**
 * \brief create a new data block to hold more data, no matter how
 */
void MemoryKV::ExpandDataBlock()
{
    std::wstringstream wss;
    wss << L"expand data block starts, currentMmfCount=" << m_currentMmfCount;
    m_logger->Log(wss.str().c_str());

    if (m_currentMmfCount < m_pHeaderBlock.GetCurrentMMFCount())
    {
        m_currentMmfCount++;
        wss.str(std::wstring());
        wss << L"skip because currentMmfCount is smaller than global mmf count, increase currentMmfCount by 1 to " << m_currentMmfCount;
        m_logger->Log(wss.str().c_str());
        return;
    }

    if(m_pHeaderBlock.GetCurrentMMFCount() >= m_options.MaxMmfCount)
    {
        m_logger->Log(L"expand data block oom");
        throw KvOomException();
    }

    // next MMF index, starts from 0 because it's C++ array index
    // so current file COUNT is next file INDEX
    int nextMmfSequence = m_pHeaderBlock.GetCurrentMMFCount();

    wchar_t* mmfName = m_pHeaderBlock.GetMmfNameAt(nextMmfSequence);
    
    auto mapSize = m_dataBlockSize * m_options.MaxBlocksPerMmf;

    auto hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        mapSize,
        mmfName);
    if (hMapFile == nullptr) 
    {
        m_logger->Log(L"MMF not created, expand failed.");
        throw std::runtime_error("MMF not created, expand failed.");
    }
    int error = GetLastError();
    if(error == ERROR_ALREADY_EXISTS)
    {
        m_logger->Log(L"MMF already exists, expand warning, but we can keep use this MMF.");
        //throw std::runtime_error("MMF already exists, expand failed.");
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
        m_logger->Log(L"Failed to map view of memory-mapped file.");
        throw std::runtime_error("Failed to map view of memory-mapped file.");
    }

    std::memset(pMapView, 0, mapSize);
    hMapFiles[nextMmfSequence] = hMapFile;
    pMapViews[nextMmfSequence] = pMapView;
    m_pHeaderBlock.SetCurrentMMFCount(m_pHeaderBlock.GetCurrentMMFCount() + 1);
    m_currentMmfCount = m_pHeaderBlock.GetCurrentMMFCount();
    std::wstringstream ss;
    ss << L"expand data block finished, currentMmfCount = " << m_currentMmfCount;
    m_logger->Log(ss.str().data());
}

void* MemoryKV::GetDataBlock(LPVOID pMapView, int i)
{
    return static_cast<char*>(static_cast<char*>(pMapView) + i * m_dataBlockSize);
}

void MemoryKV::SyncDataBlock(int dataBlockMmfIndex)
{
    std::wstringstream ss;
    ss << L"sync data block starts, mmf index = " << dataBlockMmfIndex;
    m_logger->Log(ss.str().data());
    wchar_t* mmfName = m_pHeaderBlock.GetMmfNameAt(dataBlockMmfIndex);
    auto mapSize = m_dataBlockSize * m_options.MaxBlocksPerMmf;

    auto hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        mapSize,
        mmfName);
    if (hMapFile == nullptr)
    {
        m_logger->Log(L"MMF not created, sync failed.");
        throw std::runtime_error("MMF not created, sync failed.");
    }
    int error = GetLastError();
    if (error != ERROR_ALREADY_EXISTS)
    {
        m_logger->Log(L"MMF doesn't exists, sync failed.");
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
        m_logger->Log(L"Failed to map view of memory-mapped file.");
        throw std::runtime_error("Failed to map view of memory-mapped file.");
    }

    for (int i = 0; i < m_options.MaxBlocksPerMmf; ++i)
    {
        DataBlock block(GetDataBlock(pMapView, i));
        if (!block.IsEmpty())
        {
            long globalDbIndex = BuildGlobalDbIndex(dataBlockMmfIndex, i);
            MarkGlobalDbIndex(block.GetKey(), globalDbIndex, false);
        }
    }

    hMapFiles[dataBlockMmfIndex] = hMapFile;
    pMapViews[dataBlockMmfIndex] = pMapView;

    ss.str(std::wstring());
    ss << L"sync data block finished, mmf index = " << dataBlockMmfIndex;
    m_logger->Log(ss.str().data());
}

void MemoryKV::SyncDataBlocks()
{
    while ( m_currentMmfCount < m_pHeaderBlock.GetCurrentMMFCount())
    {
        SyncDataBlock(m_currentMmfCount);
        m_currentMmfCount++;
        std::wstringstream wss;
        wss << L"currentMmfCount = " << m_currentMmfCount;
        m_logger->Log(wss.str().data());
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
    m_highestKeyPosition = -1;
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
    ss << L"initialization starts. client_name=" << m_clientName
        << L",max_key_size = " << m_options.MaxKeySize
        << L",max_value_size=" << m_options.MaxValueSize
        << L",max_data_block_count=" << m_options.MaxBlocksPerMmf
        << L",max_mmf_count=" << m_options.MaxMmfCount
        << L",current_mmf_count = " << m_currentMmfCount
        << L",connect to DB " << m_dbName;
    m_logger->Log(ss.str().data());
    InitLocalVars();
    InitHeaderBlock();
    InitDataBlock();

    m_logger->Log(L"initialization done.");
}

MemoryKV::MemoryKV(const wchar_t* clientName, std::unique_ptr<ILogger> logger)
    : m_clientName(clientName)
    , m_logger(std::move(logger))
{
    if (!m_logger) {
        m_logger = std::make_unique<SimpleFileLogger>(clientName);
    }
    std::wstringstream wss;
    wss << L"MemoryKV instance created for client: " << m_clientName;
    m_logger->Log(wss.str().c_str());
}


void MemoryKV::Open(const wchar_t* dbName, ConfigOptions options)
{
    if (dbName == nullptr || !options.Validate())
        throw KvInvalidOptionsException();

    if (IsInitialized())
        throw KvMultiInitializationException();

    m_dbName = dbName;
    m_options = options;
    m_logger->SetLogLevel(m_options.LogLevel);
    m_pHeaderBlock.SetConfigOptions(options);

    InitMutex();
    SYNC_CALL(InitializeData())
}

bool MemoryKV::IsInitialized() const
{
    return pMapViews != nullptr && hMapFiles != nullptr;
}

MemoryKV::~MemoryKV()
{
    m_pHeaderBlock.TearDown();

    if (IsInitialized())
    {
        for (int i = 0; i < m_options.MaxMmfCount; i++)
        {
            if (pMapViews[i] != nullptr)
                UnmapViewOfFile(pMapViews[i]);
            if (hMapFiles[i] != nullptr)
                CloseHandle(hMapFiles[i]);
        }
        delete[] pMapViews;
        pMapViews = nullptr;
        delete[] hMapFiles;
        hMapFiles = nullptr;
    }    
    CloseHandle(m_hMutex);  // Clean up the mutex handle
}

LPVOID MemoryKV::TheCurrentMapView() const
{
    return pMapViews[m_pHeaderBlock.GetCurrentMMFCount() - 1];
}

long MemoryKV::BuildGlobalDbIndex(int dataBlockmmfIndex, int dataBlockIndex) const
{
    return dataBlockmmfIndex * m_options.MaxBlocksPerMmf + dataBlockIndex;
}

void* MemoryKV::GetDataBlock(int dataBlockMmfIndex, int dataBlockIndex) const
{
    return static_cast<char*>(pMapViews[dataBlockMmfIndex]) + dataBlockIndex * m_dataBlockSize;
}

void MemoryKV::RefreshGlobalDbIndex()
{
    std::wstringstream wss;
    wss << L"refresh global db index begin, current HKP=" << m_highestKeyPosition<<",globalHKP="<<m_pHeaderBlock.GetHighestGlobalDbPosition();
    m_logger->Log(wss.str().c_str());
    int mmfIndex = 0;
    int blockIndex = -1;
    CrackGlobalDbIndex(m_highestKeyPosition, mmfIndex, blockIndex);

    int myCurrentMmfIndex = m_currentMmfCount-1;
    if(mmfIndex != myCurrentMmfIndex)
    {
        wss.str(std::wstring());
        wss << L"global db index inconsistent warning, HKP_mmfIndex=" << mmfIndex << ",currentMmfIndex=" << myCurrentMmfIndex <<", probably due to datablock shrink, let's sync to global HPK";
        m_logger->Log(wss.str().c_str());
        myCurrentMmfIndex = mmfIndex;
        //throw std::runtime_error("global db index inconsistent error!");
    }

    int highestMmfIndex=0;
    int highestBlockIndex=-1;
    CrackGlobalDbIndex(m_pHeaderBlock.GetHighestGlobalDbPosition(),highestMmfIndex, highestBlockIndex);

    const int targetRefreshBlockIndex = (highestMmfIndex > myCurrentMmfIndex) ?
                                            m_options.MaxBlocksPerMmf-1 : // refresh to the end of current MMF because global Db is already in next Mmf
                                            highestBlockIndex; // global write cursor still in current Mmf

    for (int i = blockIndex+1; i<= targetRefreshBlockIndex; i++)
    {
        DataBlock block(GetDataBlock(mmfIndex, i));
        if(!block.IsEmpty())
        {
            long globalDbIndex = BuildGlobalDbIndex(mmfIndex, i);
            MarkGlobalDbIndex(block.GetKey(), globalDbIndex, false);
            wss.str(std::wstring());
            wss << L"refresh mmfIndex=" << myCurrentMmfIndex << ",dataBlockIndex=" << i;
            m_logger->Log(wss.str().c_str());
        }
    }

    // sync new Mmf if any
    SyncDataBlocks();

    m_logger->Log(L"refresh global db index end");
}

/**
 * \brief 
 * \param key 
 * \param globalDbIndex 
 * \param isKeyFirstAdded true for new added key, false for keys updated from other instances
 */
void MemoryKV::MarkGlobalDbIndex(const wchar_t* key, long globalDbIndex, bool isKeyFirstAdded)
{
    std::wstringstream wss;
    wss << L"mark globalDbIndex=" << globalDbIndex << L",HKP=" << m_highestKeyPosition << L",globalHKP=" << m_pHeaderBlock.GetHighestGlobalDbPosition()<<L",firstadded="<<isKeyFirstAdded;
    m_logger->Log(wss.str().c_str());
    m_keyPositionMap[key] = globalDbIndex;
    if (globalDbIndex > m_highestKeyPosition)
    {
        m_highestKeyPosition = globalDbIndex;
        wss.str(std::wstring());
        wss << L"set HKP to " << globalDbIndex;
        m_logger->Log(wss.str().c_str());
    }
    if (isKeyFirstAdded) // this is the first time the key is created in this machine, increase the global DbPosition
    {
        if (globalDbIndex > m_pHeaderBlock.GetHighestGlobalDbPosition())
        {
            m_pHeaderBlock.SetHighestGlobalDbPosition(globalDbIndex);
            wss.str(std::wstring());
            wss << L"set globalHKP to " << globalDbIndex;
            m_logger->Log(wss.str().c_str());
        }
    }
}

void MemoryKV::UnmarkGlobalDbIndex(const std::wstring& key, int data_block_mmf_index, int data_block_index, bool isRemovedByMe)
{
    long globalDbIndex = BuildGlobalDbIndex(data_block_mmf_index, data_block_index);

    std::wstringstream wss;
    wss << L"Unmark globalDbIndex=" << globalDbIndex << L",HKP=" << m_highestKeyPosition << L",globalHKP=" << m_pHeaderBlock.GetHighestGlobalDbPosition()<<L",removedByMe="<<isRemovedByMe;
    m_logger->Log(wss.str().c_str());

    m_keyPositionMap.erase(key);
    if(globalDbIndex > m_highestKeyPosition ||
        globalDbIndex > m_pHeaderBlock.GetHighestGlobalDbPosition())
    {
        throw std::runtime_error("this should never happen");
    }

    if (globalDbIndex == m_highestKeyPosition)
    {
        m_highestKeyPosition--;
        wss.str(std::wstring());
        wss<<L"Removing the last one of local dict, reduce the local HKP by 1 to " << m_highestKeyPosition;
        m_logger->Log(wss.str().c_str());

        if (data_block_index == 0)
        {
            // if we just delete the first data block of the mmf, we need to jump back the mmf count to the last one
            m_currentMmfCount--;
            wss.str(std::wstring());
            wss << L"Removing the first one of current mmf, reduce the currentMmfCount by 1 to " << m_currentMmfCount;
            m_logger->Log(wss.str().c_str());
        }

    }

    if (isRemovedByMe) 
    {
        if (globalDbIndex == m_pHeaderBlock.GetHighestGlobalDbPosition())
        {
        //      // I should update the global Db index, this is dangerous so skip it, assume it's very low impact
        //    m_pHeaderBlock.SetHighestGlobalDbPosition(globalDbIndex - 1);
        //    wss.str(std::wstring());
        //    wss << L"I am removing the last one from global DB, reduce the global highest DB index to " << globalDbIndex - 1;
        //    m_logger->Log(wss.str().c_str());
            m_logger->Log(L"skip reduce globalHKP");
        }
    }
}

bool MemoryKV::UpdateKeyValue(const std::wstring& key, const std::wstring& value)
{
    std::wstringstream ss;
    ss << L"Put key=" << key.c_str() << L",value=" << value.c_str();
    m_logger->Log(ss.str().data());

    if (!IsInitialized())
    {
        m_logger->Log(L"[Error]. KV is not initialized");
        return false;
    }

    if(key.empty())
    {
        m_logger->Log(L"[Error]. Key is empty.");
        return false;
    }

    if (static_cast<int>(key.size()) >= m_options.MaxKeySize || 
        static_cast<int>(value.size()) >= m_options.MaxValueSize) 
    {
        m_logger->Log(L"[Error]. Key or value is too large.");
        return false;
    }

    try
    {
        int dataBlockMmfIndex;
        int dataBlockIndex;
        _FetchAndFindTheBlock(key, dataBlockMmfIndex, dataBlockIndex);
        if (dataBlockMmfIndex == -1 || dataBlockIndex == -1) // not exist till now, create new
        {
            dataBlockIndex = FindNextAvailableBlock();
            if (dataBlockIndex >= m_options.MaxBlocksPerMmf)
            {
                ExpandDataBlock();
                dataBlockIndex = FindNextAvailableBlock() % m_options.MaxBlocksPerMmf; // after data block expansion, it should never be full
            }
            dataBlockMmfIndex = m_pHeaderBlock.GetCurrentMMFCount() - 1;
            long globalDbIndex = BuildGlobalDbIndex(dataBlockMmfIndex, dataBlockIndex);

            MarkGlobalDbIndex(key.c_str(), globalDbIndex, true);
            ss.str(std::wstring());
            ss << L"find new slot. mmf index=" << dataBlockMmfIndex << L",data block index=" << dataBlockIndex;
            m_logger->Log(ss.str().data());
        }
        else //key exist before, verify its key-position is correct
        {
            ss.str(std::wstring());
            ss << L"find existing slot. mmf index=" << dataBlockMmfIndex << L",data block index=" << dataBlockIndex;
            m_logger->Log(ss.str().data());

            DataBlock block(GetDataBlock(dataBlockMmfIndex, dataBlockIndex));
            BlockState state = ValidateBlock(block, key);

            if (state == Empty) //this key was added before, but removed by somebody from another process, now need to add it again
            {
                UnmarkGlobalDbIndex(key, dataBlockMmfIndex, dataBlockIndex, false);
                ss.str(std::wstring());
                ss << L"block has been removed, unmark db index and add it back again.";
                m_logger->Log(ss.str().data());
                return UpdateKeyValue(key, value);
            }
        }

        DataBlock block = GetDataBlock(dataBlockMmfIndex, dataBlockIndex);
        block.SetKey(key.c_str(), m_options.MaxKeySize);
        block.SetValue(value.c_str(), m_options.MaxKeySize, m_options.MaxValueSize);
        m_logger->Log(L"put value successfully");
        return true;
    
    }
    catch(const KvOomException& e)
    {
        return false;
    }
}

bool MemoryKV::Put(const std::wstring& key, const std::wstring& value)
{
    bool result;
    SYNC_CALL(result = UpdateKeyValue(key, value))
    return result;
}

void MemoryKV::CrackGlobalDbIndex(long globalDbIndex, int& dataBlockMmfIndex, int& dataBlockIndex) const
{
    if(globalDbIndex <0)
    {
        dataBlockMmfIndex = 0;
        dataBlockIndex = -1;
    }
    else
    {
        dataBlockMmfIndex = static_cast<int>(globalDbIndex / m_options.MaxBlocksPerMmf);
        dataBlockIndex = static_cast<int>(globalDbIndex % m_options.MaxBlocksPerMmf);
    }
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

/**
 * \brief this method is heavy, should only be called when public request is received
 * \param key 
 * \param dataBlockMmfIndex 
 * \param dataBlockIndex 
 */
void MemoryKV::_FetchAndFindTheBlock(const std::wstring& key, int& dataBlockMmfIndex, int& dataBlockIndex)
{
    RetrieveGlobalDbIndexByKey(key, dataBlockMmfIndex, dataBlockIndex);
    if (dataBlockMmfIndex == -1 || dataBlockIndex == -1) // not exist before, refresh in case somebody added it recently
    {
        RefreshGlobalDbIndex();
        RetrieveGlobalDbIndexByKey(key, dataBlockMmfIndex, dataBlockIndex);
    }
}

void MemoryKV::QueryValueByKey(const std::wstring& key, const wchar_t*& result)
{
    std::wstringstream ss;
    ss << L"Get key=" << key.c_str();

    if(!IsInitialized())
    {
        result = L"";
        ss << L"\n[Error]. KV is not initialized";
        m_logger->Log(ss.str().data());
        return;
    }

    if (key.empty() || static_cast<int>(key.size()) >= m_options.MaxKeySize)
    {
        result = L"";
        ss << L"\n[Error]. Key size wrong.";
        m_logger->Log(ss.str().data());
        return;
    }
    
    int dataBlockMmfIndex;
    int dataBlockIndex;
    _FetchAndFindTheBlock(key, dataBlockMmfIndex, dataBlockIndex);
    if(dataBlockMmfIndex == -1 || dataBlockIndex == -1) // not found
    {
        result = L"";
        ss << L". not found";
        m_logger->Log(ss.str().data());
    }
    else 
    {
        ss << L". find the slot: mmf index=" << dataBlockMmfIndex << L",data block index=" << dataBlockIndex;
        DataBlock block(GetDataBlock(dataBlockMmfIndex, dataBlockIndex));
        m_logger->Log(ss.str().data());

        auto state = ValidateBlock(block, key);

        if (state == BlockState::Normal)
        {
            //ss.str(std::wstring());
            //ss << L"value=" << block.GetValue(m_options.MaxKeySize);
            //m_logger->Log(ss.str().data());
            result = block.GetValue(m_options.MaxKeySize);
        }
        else if(state == BlockState::Empty)
        {
            result = L"";
            UnmarkGlobalDbIndex(key, dataBlockMmfIndex, dataBlockIndex, false);

            ss.str(std::wstring());
            ss << L"block has been removed, query failed.";
            m_logger->Log(ss.str().data());
        }
    }
}

const wchar_t* MemoryKV::Get(const std::wstring& key)
{
    const wchar_t* result;
    SYNC_CALL(QueryValueByKey(key, result))
    return result;
}

void MemoryKV::RemoveData(DataBlock& block) const
{
    block.SetKey(L"", m_options.MaxKeySize);
    block.SetValue(L"", m_options.MaxKeySize, m_options.MaxValueSize);
}

BlockState MemoryKV::ValidateBlock(DataBlock& block, const std::wstring& key)
{
    if (block.IsEmpty())
    {
        m_logger->Log(L"the block is empty");
        return BlockState::Empty;
    }
    if (std::wcsncmp(block.GetKey(), key.c_str(), m_options.MaxKeySize) != 0)
    {
        std::wstringstream wss;
        wss << L". mismatched key and position. the key in block is " << block.GetKey()<<", the input key is " <<key;
        m_logger->Log(wss.str().data());
        throw std::runtime_error("key and position doesn't match");
    }
    return BlockState::Normal;
}

void MemoryKV::RemoveBlockByKey(const std::wstring& key)
{
    std::wstringstream ss;
    ss << L"Remove key=" << key.c_str();

    if(!IsInitialized())
    {
        ss << L"\n[Error] KV is not initialized";
        m_logger->Log(ss.str().data());
        return;
    }

    int dataBlockMmfIndex;
    int dataBlockIndex;
    _FetchAndFindTheBlock(key, dataBlockMmfIndex, dataBlockIndex);
    if (dataBlockMmfIndex == -1 || dataBlockIndex == -1) // not found
    {
        ss << L". not found, probably already removed.";
        m_logger->Log(ss.str().data());
    }
    else // found it, need to remove
    {
        ss << L". find the slot: mmf index=" << dataBlockMmfIndex << L",data block index=" << dataBlockIndex;
        DataBlock block(GetDataBlock(dataBlockMmfIndex, dataBlockIndex));
        m_logger->Log(ss.str().data());

        ValidateBlock(block, key);
        ss.str(std::wstring());
        ss << L"value=" << block.GetValue(m_options.MaxKeySize) << " is removed.";

        RemoveData(block);
        UnmarkGlobalDbIndex(key, dataBlockMmfIndex, dataBlockIndex, true);
        m_logger->Log(ss.str().data());
    }
}

void MemoryKV::Remove(const std::wstring& key)
{
    SYNC_CALL(RemoveBlockByKey(key))
}
