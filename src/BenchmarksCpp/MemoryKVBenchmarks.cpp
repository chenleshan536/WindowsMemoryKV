#include <benchmark/benchmark.h>
#include "MemoryKVLib.h"
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <chrono>
#include <atomic>

class MemoryKVFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        kv = new MemoryKV();
        kv->Initialize("benchmark_kv", 64, 256, 100, 1000);
    }

    void TearDown(const ::benchmark::State& state) override {
        delete kv;
    }

    MemoryKV* kv;
};

BENCHMARK_DEFINE_F(MemoryKVFixture, SetOperation)(benchmark::State& state) {
    for (auto _ : state) {
        kv->Set("benchmark_key", "benchmark_value");
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, GetOperation)(benchmark::State& state) {
    char buffer[256];
    for (auto _ : state) {
        kv->Get("benchmark_key", buffer, sizeof(buffer));
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, DeleteOperation)(benchmark::State& state) {
    for (auto _ : state) {
        kv->Delete("benchmark_key");
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, ConcurrentOperations)(benchmark::State& state) {
    const int threadCount = 4;
    const int operationsPerThread = 1000;
    std::vector<std::thread> threads;

    for (auto _ : state) {
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([this, i, operationsPerThread]() {
                for (int j = 0; j < operationsPerThread; ++j) {
                    std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
                    std::string value = "value_" + std::to_string(i) + "_" + std::to_string(j);
                    
                    kv->Set(key.c_str(), value.c_str());
                    
                    char buffer[256];
                    kv->Get(key.c_str(), buffer, sizeof(buffer));
                    
                    kv->Delete(key.c_str());
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, LargeDataOperations)(benchmark::State& state) {
    const int dataSize = 1024 * 1024; // 1MB
    std::vector<char> largeData(dataSize, 'a');
    std::vector<char> buffer(dataSize);

    for (auto _ : state) {
        kv->Set("large_data_key", largeData.data());
        kv->Get("large_data_key", buffer.data(), buffer.size());
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, BatchOperations)(benchmark::State& state) {
    const int batchSize = 1000;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    std::vector<char> buffer(256);

    for (int i = 0; i < batchSize; ++i) {
        keys.push_back("batch_key_" + std::to_string(i));
        values.push_back("batch_value_" + std::to_string(i));
    }

    for (auto _ : state) {
        // 批量写入
        for (int i = 0; i < batchSize; ++i) {
            kv->Set(keys[i].c_str(), values[i].c_str());
        }

        // 批量读取
        for (int i = 0; i < batchSize; ++i) {
            kv->Get(keys[i].c_str(), buffer.data(), buffer.size());
        }

        // 批量删除
        for (int i = 0; i < batchSize; ++i) {
            kv->Delete(keys[i].c_str());
        }
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, MultiClientConcurrentAccess)(benchmark::State& state) {
    const int clientCount = 4;
    const int operationsPerClient = 1000;
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<MemoryKV>> clients;

    // 创建多个客户端实例
    for (int i = 0; i < clientCount; ++i) {
        auto client = std::make_unique<MemoryKV>();
        client->Initialize("benchmark_kv", 64, 256, 100, 1000);
        clients.push_back(std::move(client));
    }

    for (auto _ : state) {
        // 每个客户端在单独的线程中执行操作
        for (int i = 0; i < clientCount; ++i) {
            threads.emplace_back([&clients, i, operationsPerClient]() {
                auto& client = clients[i];
                for (int j = 0; j < operationsPerClient; ++j) {
                    std::string key = "client_" + std::to_string(i) + "_key_" + std::to_string(j);
                    std::string value = "client_" + std::to_string(i) + "_value_" + std::to_string(j);
                    
                    // 写入操作
                    client->Set(key.c_str(), value.c_str());
                    
                    // 读取操作
                    char buffer[256];
                    client->Get(key.c_str(), buffer, sizeof(buffer));
                    
                    // 删除操作
                    client->Delete(key.c_str());
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, SharedKeyConcurrentAccess)(benchmark::State& state) {
    const int clientCount = 4;
    const int operationsPerClient = 1000;
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<MemoryKV>> clients;

    // 创建多个客户端实例
    for (int i = 0; i < clientCount; ++i) {
        auto client = std::make_unique<MemoryKV>();
        client->Initialize("benchmark_kv", 64, 256, 100, 1000);
        clients.push_back(std::move(client));
    }

    for (auto _ : state) {
        // 每个客户端在单独的线程中执行操作
        for (int i = 0; i < clientCount; ++i) {
            threads.emplace_back([&clients, i, operationsPerClient]() {
                auto& client = clients[i];
                for (int j = 0; j < operationsPerClient; ++j) {
                    // 所有客户端访问相同的键
                    const char* sharedKey = "shared_key";
                    std::string value = "value_" + std::to_string(i) + "_" + std::to_string(j);
                    
                    // 写入操作
                    client->Set(sharedKey, value.c_str());
                    
                    // 读取操作
                    char buffer[256];
                    client->Get(sharedKey, buffer, sizeof(buffer));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, DynamicClientCount)(benchmark::State& state) {
    const int maxClientCount = 8;
    const int operationsPerClient = 1000;
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<MemoryKV>> clients;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, maxClientCount);

    for (auto _ : state) {
        // 随机生成客户端数量
        int currentClientCount = dis(gen);
        
        // 创建客户端实例
        for (int i = 0; i < currentClientCount; ++i) {
            auto client = std::make_unique<MemoryKV>();
            client->Initialize("benchmark_kv", 64, 256, 100, 1000);
            clients.push_back(std::move(client));
        }

        // 每个客户端在单独的线程中执行操作
        for (int i = 0; i < currentClientCount; ++i) {
            threads.emplace_back([&clients, i, operationsPerClient]() {
                auto& client = clients[i];
                for (int j = 0; j < operationsPerClient; ++j) {
                    std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
                    std::string value = "value_" + std::to_string(i) + "_" + std::to_string(j);
                    
                    client->Set(key.c_str(), value.c_str());
                    char buffer[256];
                    client->Get(key.c_str(), buffer, sizeof(buffer));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
        clients.clear();
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, ClientReconnection)(benchmark::State& state) {
    const int clientCount = 4;
    const int operationsPerClient = 1000;
    const int reconnectInterval = 100; // 每100次操作后重新连接
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<MemoryKV>> clients;

    for (auto _ : state) {
        // 创建初始客户端实例
        for (int i = 0; i < clientCount; ++i) {
            auto client = std::make_unique<MemoryKV>();
            client->Initialize("benchmark_kv", 64, 256, 100, 1000);
            clients.push_back(std::move(client));
        }

        // 每个客户端在单独的线程中执行操作
        for (int i = 0; i < clientCount; ++i) {
            threads.emplace_back([&clients, i, operationsPerClient, reconnectInterval]() {
                for (int j = 0; j < operationsPerClient; ++j) {
                    // 定期重新连接
                    if (j % reconnectInterval == 0) {
                        clients[i] = std::make_unique<MemoryKV>();
                        clients[i]->Initialize("benchmark_kv", 64, 256, 100, 1000);
                    }

                    auto& client = clients[i];
                    std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
                    std::string value = "value_" + std::to_string(i) + "_" + std::to_string(j);
                    
                    client->Set(key.c_str(), value.c_str());
                    char buffer[256];
                    client->Get(key.c_str(), buffer, sizeof(buffer));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
        clients.clear();
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, MixedReadWriteOperations)(benchmark::State& state) {
    const int clientCount = 4;
    const int operationsPerClient = 1000;
    const int writeRatio = state.range(0); // 写入操作的比例（0-100）
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<MemoryKV>> clients;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);

    // 创建多个客户端实例
    for (int i = 0; i < clientCount; ++i) {
        auto client = std::make_unique<MemoryKV>();
        client->Initialize("benchmark_kv", 64, 256, 100, 1000);
        clients.push_back(std::move(client));
    }

    // 预填充一些数据
    for (int i = 0; i < 1000; ++i) {
        std::string key = "prefill_key_" + std::to_string(i);
        std::string value = "prefill_value_" + std::to_string(i);
        clients[0]->Set(key.c_str(), value.c_str());
    }

    for (auto _ : state) {
        // 每个客户端在单独的线程中执行操作
        for (int i = 0; i < clientCount; ++i) {
            threads.emplace_back([&clients, i, operationsPerClient, writeRatio, &dis, &gen]() {
                auto& client = clients[i];
                for (int j = 0; j < operationsPerClient; ++j) {
                    int opType = dis(gen);
                    if (opType < writeRatio) {
                        // 写入操作
                        std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
                        std::string value = "value_" + std::to_string(i) + "_" + std::to_string(j);
                        client->Set(key.c_str(), value.c_str());
                    } else {
                        // 读取操作
                        std::string key = "prefill_key_" + std::to_string(j % 1000);
                        char buffer[256];
                        client->Get(key.c_str(), buffer, sizeof(buffer));
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, RandomReadWriteOperations)(benchmark::State& state) {
    const int clientCount = 4;
    const int operationsPerClient = 1000;
    const int keySpace = 1000; // 键空间大小
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<MemoryKV>> clients;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> keyDis(0, keySpace - 1);
    std::uniform_int_distribution<> opDis(0, 1);

    // 创建多个客户端实例
    for (int i = 0; i < clientCount; ++i) {
        auto client = std::make_unique<MemoryKV>();
        client->Initialize("benchmark_kv", 64, 256, 100, 1000);
        clients.push_back(std::move(client));
    }

    for (auto _ : state) {
        // 每个客户端在单独的线程中执行操作
        for (int i = 0; i < clientCount; ++i) {
            threads.emplace_back([&clients, i, operationsPerClient, &keyDis, &opDis, &gen]() {
                auto& client = clients[i];
                for (int j = 0; j < operationsPerClient; ++j) {
                    int keyIndex = keyDis(gen);
                    std::string key = "key_" + std::to_string(keyIndex);
                    
                    if (opDis(gen) == 0) {
                        // 写入操作
                        std::string value = "value_" + std::to_string(i) + "_" + std::to_string(j);
                        client->Set(key.c_str(), value.c_str());
                    } else {
                        // 读取操作
                        char buffer[256];
                        client->Get(key.c_str(), buffer, sizeof(buffer));
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
    }
}

BENCHMARK_DEFINE_F(MemoryKVFixture, HotspotAccessPattern)(benchmark::State& state) {
    const int clientCount = 4;
    const int operationsPerClient = 1000;
    const int hotspotCount = 10; // 热点数据数量
    const int hotspotRatio = 80; // 热点数据访问比例（%）
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<MemoryKV>> clients;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> hotspotDis(0, hotspotCount - 1);
    std::uniform_int_distribution<> normalDis(hotspotCount, 999);
    std::uniform_int_distribution<> ratioDis(0, 99);

    // 创建多个客户端实例
    for (int i = 0; i < clientCount; ++i) {
        auto client = std::make_unique<MemoryKV>();
        client->Initialize("benchmark_kv", 64, 256, 100, 1000);
        clients.push_back(std::move(client));
    }

    // 预填充热点数据
    for (int i = 0; i < hotspotCount; ++i) {
        std::string key = "hotspot_key_" + std::to_string(i);
        std::string value = "hotspot_value_" + std::to_string(i);
        clients[0]->Set(key.c_str(), value.c_str());
    }

    for (auto _ : state) {
        // 每个客户端在单独的线程中执行操作
        for (int i = 0; i < clientCount; ++i) {
            threads.emplace_back([&clients, i, operationsPerClient, hotspotRatio, 
                                &hotspotDis, &normalDis, &ratioDis, &gen]() {
                auto& client = clients[i];
                for (int j = 0; j < operationsPerClient; ++j) {
                    std::string key;
                    if (ratioDis(gen) < hotspotRatio) {
                        // 访问热点数据
                        key = "hotspot_key_" + std::to_string(hotspotDis(gen));
                    } else {
                        // 访问普通数据
                        key = "normal_key_" + std::to_string(normalDis(gen));
                    }
                    
                    char buffer[256];
                    client->Get(key.c_str(), buffer, sizeof(buffer));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
    }
}

BENCHMARK_REGISTER_F(MemoryKVFixture, SetOperation);
BENCHMARK_REGISTER_F(MemoryKVFixture, GetOperation);
BENCHMARK_REGISTER_F(MemoryKVFixture, DeleteOperation);
BENCHMARK_REGISTER_F(MemoryKVFixture, ConcurrentOperations);
BENCHMARK_REGISTER_F(MemoryKVFixture, LargeDataOperations);
BENCHMARK_REGISTER_F(MemoryKVFixture, BatchOperations);
BENCHMARK_REGISTER_F(MemoryKVFixture, MultiClientConcurrentAccess);
BENCHMARK_REGISTER_F(MemoryKVFixture, SharedKeyConcurrentAccess);
BENCHMARK_REGISTER_F(MemoryKVFixture, DynamicClientCount);
BENCHMARK_REGISTER_F(MemoryKVFixture, ClientReconnection);
BENCHMARK_REGISTER_F(MemoryKVFixture, MixedReadWriteOperations)
    ->Arg(20)  // 20% 写入
    ->Arg(50)  // 50% 写入
    ->Arg(80); // 80% 写入
BENCHMARK_REGISTER_F(MemoryKVFixture, RandomReadWriteOperations);
BENCHMARK_REGISTER_F(MemoryKVFixture, HotspotAccessPattern);

BENCHMARK_MAIN(); 