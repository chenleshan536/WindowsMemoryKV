#include "pch.h"
#include <gtest/gtest.h>
#include "../MemoryKVLib/MemoryKV.h"
#include <windows.h>
#include <psapi.h>
#include <vector>
#include <thread>

#include "MockLogger.h"

class MemoryLeakTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 获取初始内存使用
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
        initialMemory = pmc.WorkingSetSize;
    }

    void TearDown() override {
        // 检查内存泄漏
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
        size_t finalMemory = pmc.WorkingSetSize;
        
        // x86 平台允许 2MB 的内存增长容忍度
        const size_t memoryTolerance = 2 * 1024 * 1024; // 2MB
        EXPECT_LE(finalMemory - initialMemory, memoryTolerance);
    }

    size_t initialMemory;
};

TEST_F(MemoryLeakTest, BasicOperationsLeak) {
    // 减少迭代次数以适应 x86 平台
    const int iterations = 5000;
    for (int i = 0; i < iterations; ++i) {
        MemoryKV* kv = new MemoryKV(L"test_client", std::make_unique<MockLogger>());
        
        ConfigOptions options;
        options.MaxKeySize = 64;
        options.MaxValueSize = 256;
        options.MaxBlocksPerMmf = 100;
        options.MaxMmfCount = 100;
        options.LogLevel = 0;
        
        kv->Open(L"BasicOperationsLeak", options);
        
        // 使用较小的数据块
        std::wstring key = L"key" + std::to_wstring(i);
        std::wstring value = L"value" + std::to_wstring(i);
        kv->Put(key, value);
        
        const wchar_t* result = kv->Get(key);
        EXPECT_STREQ(result, value.c_str());
        kv->Remove(key);
        
        delete kv;
    }
}

TEST_F(MemoryLeakTest, LargeDataLeak) {
    // 减少数据大小和迭代次数
    const int dataSize = 512 * 1024; // 512KB
    const int iterations = 100;
    
    std::vector<wchar_t> largeValue(dataSize, L'A');
    largeValue[dataSize - 1] = L'\0';
    
    for (int i = 0; i < iterations; ++i) {
        MemoryKV* kv = new MemoryKV(L"test_client", std::make_unique<MockLogger>());
        
        ConfigOptions options;
        options.MaxKeySize = 64;
        options.MaxValueSize = dataSize;
        options.MaxBlocksPerMmf = 5;
        options.MaxMmfCount = 50;
        options.LogLevel = 0;
        
        kv->Open(L"LargeDataLeak", options);
        
        std::wstring key = L"large_key" + std::to_wstring(i);
        kv->Put(key, largeValue.data());
        
        const wchar_t* result = kv->Get(key);
        EXPECT_STREQ(result, largeValue.data());
        kv->Remove(key);
        
        delete kv;
    }
}

TEST_F(MemoryLeakTest, ConcurrentLeak) {
    // 减少线程数和迭代次数
    const int threadCount = 2;
    const int iterations = 500;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([iterations, i]() {
            for (int j = 0; j < iterations; ++j) {
                MemoryKV* kv = new MemoryKV(L"test_client", std::make_unique<MockLogger>());
                
                ConfigOptions options;
                options.MaxKeySize = 64;
                options.MaxValueSize = 256;
                options.MaxBlocksPerMmf = 100;
                options.MaxMmfCount = 50;
                options.LogLevel = 0;
                
                kv->Open(L"ConcurrentLeak", options);
                
                std::wstring key = L"thread" + std::to_wstring(i) + L"_key" + std::to_wstring(j);
                std::wstring value = L"value" + std::to_wstring(j);
                kv->Put(key, value);
                
                const wchar_t* result = kv->Get(key);
                EXPECT_STREQ(result, value.c_str());
                kv->Remove(key);
                
                delete kv;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
} 