using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Running;
using MemoryKVLib.Net;
using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Threading;

namespace Benchmarks
{
    [MemoryDiagnoser]
    [ThreadingDiagnoser]
    public class MemoryKVBenchmarks
    {
        private MemoryKVClient _client;
        private const string ServiceName = "benchmark_kv";
        private const int KeyLength = 64;
        private const int ValueLength = 256;
        private const int SmallBatchSize = 100;  // 减少批处理大小
        private const int LargeDataSize = 512 * 1024;  // 减少大数据大小到 512KB

        [GlobalSetup]
        public void Setup()
        {
            _client = new MemoryKVClient();
            _client.Initialize("TestService", KeyLength, ValueLength, 5, 50);  // 减少内存映射文件数量和块数
        }

        [GlobalCleanup]
        public void Cleanup()
        {
            _client.Dispose();
        }

        [Benchmark]
        public void SetOperation()
        {
            for (int i = 0; i < 1000; i++)  // 减少迭代次数
            {
                string key = $"key_{i}";
                string value = $"value_{i}";
                _client.Set(key, value);
            }
        }

        [Benchmark]
        public void GetOperation()
        {
            for (int i = 0; i < 1000; i++)  // 减少迭代次数
            {
                string key = $"key_{i}";
                _client.Get(key);
            }
        }

        [Benchmark]
        public void DeleteOperation()
        {
            for (int i = 0; i < 1000; i++)  // 减少迭代次数
            {
                string key = $"key_{i}";
                _client.Delete(key);
            }
        }

        [Benchmark]
        public void ConcurrentOperations()
        {
            const int threadCount = 2;  // 减少线程数
            var threads = new Thread[threadCount];
            for (int i = 0; i < threadCount; i++)
            {
                threads[i] = new Thread(() =>
                {
                    for (int j = 0; j < 500; j++)  // 减少每个线程的迭代次数
                    {
                        string key = $"thread_{i}_key_{j}";
                        string value = $"value_{j}";
                        _client.Set(key, value);
                        _client.Get(key);
                        _client.Delete(key);
                    }
                });
                threads[i].Start();
            }
            foreach (var thread in threads)
            {
                thread.Join();
            }
        }

        [Benchmark]
        public void LargeDataOperations()
        {
            var largeData = new string('A', LargeDataSize);
            for (int i = 0; i < 10; i++)  // 减少迭代次数
            {
                string key = $"large_key_{i}";
                _client.Set(key, largeData);
                _client.Get(key);
                _client.Delete(key);
            }
        }

        [Benchmark]
        public void BatchOperations()
        {
            var keys = new string[SmallBatchSize];
            var values = new string[SmallBatchSize];
            for (int i = 0; i < SmallBatchSize; i++)
            {
                keys[i] = $"batch_key_{i}";
                values[i] = $"batch_value_{i}";
            }

            // 批量设置
            for (int i = 0; i < SmallBatchSize; i++)
            {
                _client.Set(keys[i], values[i]);
            }

            // 批量获取
            for (int i = 0; i < SmallBatchSize; i++)
            {
                _client.Get(keys[i]);
            }

            // 批量删除
            for (int i = 0; i < SmallBatchSize; i++)
            {
                _client.Delete(keys[i]);
            }
        }

        [Benchmark]
        [Arguments(20)]  // 20% 写入
        [Arguments(50)]  // 50% 写入
        [Arguments(80)]  // 80% 写入
        public void MixedReadWriteOperations(int writeRatio)
        {
            const int clientCount = 4;
            const int operationsPerClient = 1000;
            var clients = new List<MemoryKVClient>();
            var tasks = new Task[clientCount];
            var random = new Random();

            // 创建多个客户端实例
            for (int i = 0; i < clientCount; i++)
            {
                var client = new MemoryKVClient();
                client.Initialize(ServiceName);
                clients.Add(client);
            }

            // 预填充一些数据
            for (int i = 0; i < 1000; i++)
            {
                string key = $"prefill_key_{i}";
                string value = $"prefill_value_{i}";
                clients[0].Set(key, value);
            }

            // 每个客户端在单独的线程中执行操作
            for (int i = 0; i < clientCount; i++)
            {
                int clientIndex = i;
                tasks[i] = Task.Run(() =>
                {
                    var client = clients[clientIndex];
                    for (int j = 0; j < operationsPerClient; j++)
                    {
                        if (random.Next(100) < writeRatio)
                        {
                            // 写入操作
                            string key = $"key_{clientIndex}_{j}";
                            string value = $"value_{clientIndex}_{j}";
                            client.Set(key, value);
                        }
                        else
                        {
                            // 读取操作
                            string key = $"prefill_key_{j % 1000}";
                            client.Get(key, out _);
                        }
                    }
                });
            }

            // 等待所有任务完成
            Task.WaitAll(tasks);

            // 清理客户端
            foreach (var client in clients)
            {
                client.Dispose();
            }
        }

        [Benchmark]
        public void RandomReadWriteOperations()
        {
            const int clientCount = 4;
            const int operationsPerClient = 1000;
            const int keySpace = 1000; // 键空间大小
            var clients = new List<MemoryKVClient>();
            var tasks = new Task[clientCount];
            var random = new Random();

            // 创建多个客户端实例
            for (int i = 0; i < clientCount; i++)
            {
                var client = new MemoryKVClient();
                client.Initialize(ServiceName);
                clients.Add(client);
            }

            // 每个客户端在单独的线程中执行操作
            for (int i = 0; i < clientCount; i++)
            {
                int clientIndex = i;
                tasks[i] = Task.Run(() =>
                {
                    var client = clients[clientIndex];
                    for (int j = 0; j < operationsPerClient; j++)
                    {
                        int keyIndex = random.Next(keySpace);
                        string key = $"key_{keyIndex}";

                        if (random.Next(2) == 0)
                        {
                            // 写入操作
                            string value = $"value_{clientIndex}_{j}";
                            client.Set(key, value);
                        }
                        else
                        {
                            // 读取操作
                            client.Get(key, out _);
                        }
                    }
                });
            }

            // 等待所有任务完成
            Task.WaitAll(tasks);

            // 清理客户端
            foreach (var client in clients)
            {
                client.Dispose();
            }
        }

        [Benchmark]
        public void HotspotAccessPattern()
        {
            const int clientCount = 4;
            const int operationsPerClient = 1000;
            const int hotspotCount = 10; // 热点数据数量
            const int hotspotRatio = 80; // 热点数据访问比例（%）
            var clients = new List<MemoryKVClient>();
            var tasks = new Task[clientCount];
            var random = new Random();

            // 创建多个客户端实例
            for (int i = 0; i < clientCount; i++)
            {
                var client = new MemoryKVClient();
                client.Initialize(ServiceName);
                clients.Add(client);
            }

            // 预填充热点数据
            for (int i = 0; i < hotspotCount; i++)
            {
                string key = $"hotspot_key_{i}";
                string value = $"hotspot_value_{i}";
                clients[0].Set(key, value);
            }

            // 每个客户端在单独的线程中执行操作
            for (int i = 0; i < clientCount; i++)
            {
                int clientIndex = i;
                tasks[i] = Task.Run(() =>
                {
                    var client = clients[clientIndex];
                    for (int j = 0; j < operationsPerClient; j++)
                    {
                        string key;
                        if (random.Next(100) < hotspotRatio)
                        {
                            // 访问热点数据
                            key = $"hotspot_key_{random.Next(hotspotCount)}";
                        }
                        else
                        {
                            // 访问普通数据
                            key = $"normal_key_{random.Next(hotspotCount, 1000)}";
                        }

                        client.Get(key, out _);
                    }
                });
            }

            // 等待所有任务完成
            Task.WaitAll(tasks);

            // 清理客户端
            foreach (var client in clients)
            {
                client.Dispose();
            }
        }
    }

    public class Program
    {
        public static void Main(string[] args)
        {
            var summary = BenchmarkRunner.Run<MemoryKVBenchmarks>();
        }
    }
} 