
#include "pch.h"
#include <gtest/gtest.h>
#include "../MemoryKVLib/MemoryKV.h"
#include <string>
#include <vector>

#include "MockLogger.h"

class BoundaryTest : public ::testing::Test {
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

// 测试最大键长度
TEST_F(BoundaryTest, MaxKeyLength) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 1;
    options.LogLevel = 0;
    
    kv->Open(L"MaxKeyLength", options);
    
    // 测试最大长度键
    std::wstring maxKey(64-1, L'a');
    std::wstring value = L"test_value";
    EXPECT_TRUE(kv->Put(maxKey, value));
    
    const wchar_t* result = kv->Get(maxKey);
    EXPECT_STREQ(result, value.c_str());
    
    // 测试超长键
    std::wstring tooLongKey(64, L'a');
    EXPECT_FALSE(kv->Put(tooLongKey, value));
}

// 测试最大值长度
TEST_F(BoundaryTest, MaxValueLength) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 1;
    options.LogLevel = 0;
    
    kv->Open(L"MaxValueLength", options);
    
    // 测试最大长度值
    std::wstring key = L"test_key";
    std::wstring maxValue(256-1, L'a');
    EXPECT_TRUE(kv->Put(key, maxValue));
    
    const wchar_t* result = kv->Get(key);
    EXPECT_STREQ(result, maxValue.c_str());
    
    // 测试超长值
    std::wstring tooLongValue(256, L'a');
    EXPECT_FALSE(kv->Put(key, tooLongValue));
}

// 测试最大并发连接数
TEST_F(BoundaryTest, MaxConcurrentConnections) {
    const int maxConnections = 100;
    std::vector<MemoryKV*> clients;
    
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 1;
    options.LogLevel = 0;
    
    // 创建最大数量的连接
    for (int i = 0; i < maxConnections; ++i) {
        auto client = new MemoryKV(L"test_client", std::make_unique<MockLogger>());
        client->Open(L"MaxConcurrentConnections", options);
        clients.push_back(client);
    }
    
    // 尝试创建额外连接
    auto extraClient = new MemoryKV(L"test_client", std::make_unique<MockLogger>());
    extraClient->Open(L"MaxConcurrentConnections", options);
    delete extraClient;
    
    // 清理
    for (auto client : clients) {
        delete client;
    }
}

// 测试内存使用限制
TEST_F(BoundaryTest, MemoryUsageLimit) {
    const int maxMemoryMB = 100; // 100KB 内存限制
    const int valueSize = 256; // 1KB 值大小
    
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = valueSize;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 1; //do not allow expansion
    options.LogLevel = 0;
    
    kv->Open(L"MemoryUsageLimit", options);
    
    // 尝试写入超过内存限制的数据
    std::wstring largeValue(valueSize-1, L'a');
    for (int i = 0; i < maxMemoryMB + 1; ++i) {
        std::wstring key = L"key_" + std::to_wstring(i);
        std::cout << i << "\n";
        if (i < maxMemoryMB) {
            EXPECT_TRUE(kv->Put(key, largeValue));
        } else {
            EXPECT_FALSE(kv->Put(key, largeValue));
        }
    }
}

// 测试空键和空值
TEST_F(BoundaryTest, EmptyKeyAndValue) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 1;
    options.LogLevel = 0;
    
    kv->Open(L"EmptyKeyAndValue", options);
    
    // 测试空键
    EXPECT_FALSE(kv->Put(L"", L"value"));
    
    // 测试空值
    EXPECT_TRUE(kv->Put(L"key", L""));
    const wchar_t* result = kv->Get(L"key");
    EXPECT_STREQ(result, L"");
    kv->Remove(L"key");
}

// 测试特殊字符
TEST_F(BoundaryTest, SpecialCharacters) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 1;
    options.LogLevel = 0;
    
    kv->Open(L"SpecialCharacters", options);
    
    // 测试特殊字符键
    std::wstring specialKey = L"key!@#$%^&*()_+-=[]{}|;:'\",.<>?/\\";
    std::wstring value = L"test_value";
    EXPECT_TRUE(kv->Put(specialKey, value));
    
    const wchar_t* result = kv->Get(specialKey);
    EXPECT_STREQ(result, value.c_str());
    kv->Remove(specialKey);
    
    // 测试特殊字符值
    std::wstring key = L"test_key";
    std::wstring specialValue = L"value!@#$%^&*()_+-=[]{}|;:'\",.<>?/\\";
    EXPECT_TRUE(kv->Put(key, specialValue));
    
    result = kv->Get(key);
    EXPECT_STREQ(result, specialValue.c_str());
    kv->Remove(key);
}

// 测试缓冲区大小
TEST_F(BoundaryTest, BufferSize) {
    ConfigOptions options;
    options.MaxKeySize = 64;
    options.MaxValueSize = 256;
    options.MaxBlocksPerMmf = 100;
    options.MaxMmfCount = 1;
    options.LogLevel = 0;
    
    kv->Open(L"BufferSize", options);
    
    // 测试不同大小的键和值
    for (int i = 1; i <= 64-1; ++i) {
        std::wstring key(i, L'a');
        std::wstring value(i, L'b');
        EXPECT_TRUE(kv->Put(key, value));
        
        const wchar_t* result = kv->Get(key);
        EXPECT_STREQ(result, value.c_str());
        kv->Remove(key);
    }
} 