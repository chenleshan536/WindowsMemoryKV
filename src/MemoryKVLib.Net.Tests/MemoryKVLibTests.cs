using Xunit;
using System;
using System.Threading.Tasks;
using System.Collections.Concurrent;

namespace MemoryKVLib.Net.Tests
{
    public class MemoryKVLibTests
    {
        [Fact]
        public void BasicOperations()
        {
            using var kv = new MemoryKV();
            Assert.True(kv.Initialize("test_kv", 64, 256, 100, 1000));

            const string key = "test_key";
            const string value = "test_value";
            Assert.True(kv.Set(key, value));

            Assert.True(kv.Get(key, out string result));
            Assert.Equal(value, result);

            Assert.True(kv.Delete(key));
            Assert.False(kv.Get(key, out _));
        }

        [Fact]
        public void EdgeCases()
        {
            using var kv = new MemoryKV();
            Assert.True(kv.Initialize("test_kv_edge", 64, 256, 100, 1000));

            Assert.False(kv.Set("", "value"));
            Assert.False(kv.Set("key", ""));

            string longKey = new string('a', 65);
            Assert.False(kv.Set(longKey, "value"));

            string longValue = new string('a', 257);
            Assert.False(kv.Set("key", longValue));
        }

        [Fact]
        public async Task ConcurrentAccess()
        {
            using var kv = new MemoryKV();
            Assert.True(kv.Initialize("test_kv_concurrent", 64, 256, 100, 1000));

            const int threadCount = 4;
            const int operationsPerThread = 1000;
            var tasks = new Task[threadCount];

            for (int i = 0; i < threadCount; i++)
            {
                int threadId = i;
                tasks[i] = Task.Run(() =>
                {
                    for (int j = 0; j < operationsPerThread; j++)
                    {
                        string key = $"key_{threadId}_{j}";
                        string value = $"value_{threadId}_{j}";

                        Assert.True(kv.Set(key, value));
                        Assert.True(kv.Get(key, out string result));
                        Assert.Equal(value, result);
                        Assert.True(kv.Delete(key));
                    }
                });
            }

            await Task.WhenAll(tasks);
        }

        [Fact]
        public void PerformanceTest()
        {
            using var kv = new MemoryKV();
            Assert.True(kv.Initialize("test_kv_perf", 64, 256, 100, 1000));

            const int iterations = 10000;
            var startTime = DateTime.Now;

            for (int i = 0; i < iterations; i++)
            {
                string key = $"key_{i}";
                string value = $"value_{i}";
                Assert.True(kv.Set(key, value));
            }

            var duration = DateTime.Now - startTime;
            Console.WriteLine($"写入 {iterations} 条数据耗时: {duration.TotalMilliseconds}ms");
        }

        [Fact]
        public void StressTest()
        {
            using var kv = new MemoryKV();
            Assert.True(kv.Initialize("test_kv_stress", 64, 256, 100, 1000));

            const int maxEntries = 100000;
            var keys = new string[maxEntries];
            var values = new string[maxEntries];

            for (int i = 0; i < maxEntries; i++)
            {
                keys[i] = $"key_{i}";
                values[i] = $"value_{i}";
            }

            var random = new Random();
            for (int i = 0; i < maxEntries; i++)
            {
                int idx = random.Next(maxEntries);
                Assert.True(kv.Set(keys[idx], values[idx]));
            }

            for (int i = 0; i < 1000; i++)
            {
                int idx = random.Next(maxEntries);
                Assert.True(kv.Get(keys[idx], out string result));
                Assert.Equal(values[idx], result);
            }
        }

        [Fact]
        public void ExceptionHandling()
        {
            using var kv = new MemoryKV();

            Assert.False(kv.Initialize("", 64, 256, 100, 1000));
            Assert.False(kv.Initialize("test_kv", 0, 256, 100, 1000));
            Assert.False(kv.Initialize("test_kv", 64, 0, 100, 1000));
            Assert.False(kv.Initialize("test_kv", 64, 256, 0, 1000));
            Assert.False(kv.Initialize("test_kv", 64, 256, 100, 0));

            Assert.False(kv.Set("key", "value"));
            Assert.False(kv.Get("key", out _));
            Assert.False(kv.Delete("key"));
        }
    }
} 