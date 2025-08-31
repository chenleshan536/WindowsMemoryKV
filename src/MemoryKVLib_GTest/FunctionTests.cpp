#include "pch.h"
#include <gtest/gtest.h>
#include "../MemoryKVLib/MemoryKV.h"
#include <thread>
#include <vector>
#include <chrono>
#include <random>

#include "MockLogger.h"

class FunctionTest : public ::testing::Test {
protected:
    void SetUp() override {
        kv = new MemoryKV(L"test_client", std::make_unique<MockLogger>(true));
        std::cout << "setup\n";
    }

    void TearDown() override {
        delete kv;
    }

    MemoryKV* kv;
};

// 基本操作测试
TEST_F(FunctionTest, BasicOperations) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 100;
    options.LogLevel = 0;
    
    kv->Open(L"BasicOperations", options);
    
    std::wstring key = L"test_key";
    std::wstring value = L"test_value";
    EXPECT_TRUE(kv->Put(key, value));
    
    const wchar_t* result = kv->Get(key);
    EXPECT_STREQ(result, value.c_str());
    
    kv->Remove(key);
    EXPECT_STREQ(kv->Get(key), L"");
}

// 边界条件测试
TEST_F(FunctionTest, BoundaryConditions) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 100;
    options.LogLevel = 0;
    
    kv->Open(L"BoundaryConditions", options);
    
    // 测试空键, put will fail
    EXPECT_FALSE(kv->Put(L"", L"value"));
    
    // 测试空值, put succeed
    EXPECT_TRUE(kv->Put(L"key", L""));

    const wchar_t* result = kv->Get(L"key");
    EXPECT_STREQ(result, L"");

    //remove ok
    kv->Remove(L"key");
    
    // 测试最大长度键,it's allowed
    std::wstring max_key(64-1, L'a');
    EXPECT_TRUE(kv->Put(max_key, L"value"));

    // QUERY OK
    result = kv->Get(max_key);
    EXPECT_STREQ(result, L"value");

    //remove ok
    kv->Remove(max_key);

    // QUERY again and verify it's removed
    result = kv->Get(max_key);
    EXPECT_STREQ(result, L"");
    
    // 测试最大长度值, it's allowed
    std::wstring max_value(256-1, L'b');
    EXPECT_TRUE(kv->Put(L"key", max_value));

    //QUERY OK
    result = kv->Get(L"key");
    EXPECT_STREQ(result, max_value.c_str());

    //remove ok
    kv->Remove(L"key");

    // QUERY again and verify it's removed
    result = kv->Get(L"key");
    EXPECT_STREQ(result, L"");

    // 测试超出最大长度, it's not allowed and will be ignored
    std::wstring too_long_key(64, L'a');
    EXPECT_FALSE(kv->Put(too_long_key, L"value"));
    
    std::wstring too_long_value(256, L'b');
    EXPECT_FALSE(kv->Put(L"key", too_long_value));
    
    // 测试不存在的键
    result = kv->Get(L"nonexistent");
    EXPECT_STREQ(result, L"");
}

// 并发测试
TEST_F(FunctionTest, ConcurrentOperations) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 100;
    options.LogLevel = 0;

    for (int j= 0; j < 100; ++j)
    {
        std::cout << "test " << j << std::endl;

        auto kv2 = new MemoryKV(L"test_client", std::make_unique<MockLogger>(true));
        kv2->Open(L"ConcurrentOperations", options);

        const int num_threads = 4;
        const int num_operations = 1000;
        std::vector<std::thread> threads;

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, this, i, num_operations]() {
                for (int j = 0; j < num_operations; ++j) {
                    std::wstring key = L"key_" + std::to_wstring(i) + L"_" + std::to_wstring(j);
                    std::wstring value = L"value_" + std::to_wstring(i) + L"_" + std::to_wstring(j);

                    EXPECT_TRUE(kv2->Put(key, value));
                    const wchar_t* result = kv2->Get(key);
                    EXPECT_STREQ(result, value.c_str());
                    kv2->Remove(key);
                }
                });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        delete kv2;
    }
}

// 性能测试
TEST_F(FunctionTest, PerformanceTest) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 100;
    options.LogLevel = 0;
    
    kv->Open(L"PerformanceTest", options);
    
    const int num_operations = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_operations; ++i) {
        std::wstring key = L"key_" + std::to_wstring(i);
        std::wstring value = L"value_" + std::to_wstring(i);
        
        EXPECT_TRUE(kv->Put(key, value));
        const wchar_t* result = kv->Get(key);
        EXPECT_STREQ(result, value.c_str());
        kv->Remove(key);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Performance test completed in " << duration.count() << " ms" << std::endl;
    std::cout << "Average operation time: " << (double)duration.count() / num_operations << " ms" << std::endl;
}

// 压力测试
TEST_F(FunctionTest, StressTest) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 100;
    options.LogLevel = 0;
    
    kv->Open(L"StressTest", options);
    
    const int maxEntries = 10000;
    std::vector<std::wstring> keys;
    std::vector<std::wstring> values;
    
    // 预生成测试数据
    for (int i = 0; i < maxEntries; ++i) {
        keys.push_back(L"key_" + std::to_wstring(i));
        values.push_back(L"value_" + std::to_wstring(i));
    }
    
    // 随机写入
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, maxEntries - 1);
    
    for (int i = 0; i < maxEntries; ++i) {
        EXPECT_TRUE(kv->Put(keys[i], values[i]));
    }
    
    // 验证随机读取
    for (int i = 0; i < 1000; ++i) {
        int idx = dis(gen);
        const wchar_t* result = kv->Get(keys[idx]);
        EXPECT_STREQ(result, values[idx].c_str());
    }
}

// 异常处理测试
TEST_F(FunctionTest, ExceptionHandling) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 100;
    options.LogLevel = 0;

    // 测试无效初始化参数
    ConfigOptions invalidOptions = options;
    invalidOptions.MaxKeySize = 0;
    auto kv2 = new MemoryKV(L"test_client", std::make_unique<MockLogger>(true));
    EXPECT_THROW({ kv2->Open(L"ExceptionHandling", invalidOptions); }, KvInvalidOptionsException);
    EXPECT_STREQ(kv2->Get(L"key"), L"");

    invalidOptions = options;
    invalidOptions.MaxValueSize = 0;
    auto kv3 = new MemoryKV(L"test_client", std::make_unique<MockLogger>(true));
    EXPECT_THROW({ kv3->Open(L"ExceptionHandling", invalidOptions); }, KvInvalidOptionsException);

    invalidOptions = options;
    invalidOptions.MaxBlocksPerMmf = 0;
    auto kv4 = new MemoryKV(L"test_client", std::make_unique<MockLogger>(true));
    EXPECT_THROW({ kv4->Open(L"ExceptionHandling", invalidOptions); }, KvInvalidOptionsException);

    invalidOptions = options;
    invalidOptions.MaxMmfCount = 0;
    auto kv5 = new MemoryKV(L"test_client", std::make_unique<MockLogger>(true));
    EXPECT_THROW({ kv5->Open(L"ExceptionHandling", invalidOptions); }, KvInvalidOptionsException);

}

TEST_F(FunctionTest, OperationWithoutOpen) {
    // 测试未初始化时的操作, no exception
    EXPECT_FALSE(kv->Put(L"key", L"value"));
    EXPECT_STREQ(kv->Get(L"key"), L"");
    kv->Remove(L"key");
    EXPECT_STREQ(kv->Get(L"key"), L"");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 