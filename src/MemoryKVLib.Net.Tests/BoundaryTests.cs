using Xunit;
using MemoryKVLib.Net;
using System;
using System.Collections.Generic;
using System.Text;

namespace MemoryKVLib.Net.Tests
{
    public class BoundaryTests : IDisposable
    {
        private readonly MemoryKVClient _client;
        private const string ServiceName = "test_kv";

        public BoundaryTests()
        {
            _client = new MemoryKVClient();
            _client.Initialize(ServiceName);
        }

        public void Dispose()
        {
            _client.Dispose();
        }

        [Fact]
        public void MaxKeyLength()
        {
            // 测试最大长度键
            string maxKey = new string('a', 64);
            string value = "test_value";
            Assert.True(_client.Set(maxKey, value));
            
            Assert.True(_client.Get(maxKey, out string retrievedValue));
            Assert.Equal(value, retrievedValue);
            
            // 测试超长键
            string tooLongKey = new string('a', 65);
            Assert.False(_client.Set(tooLongKey, value));
        }

        [Fact]
        public void MaxValueLength()
        {
            // 测试最大长度值
            string key = "test_key";
            string maxValue = new string('a', 256);
            Assert.True(_client.Set(key, maxValue));
            
            Assert.True(_client.Get(key, out string retrievedValue));
            Assert.Equal(maxValue, retrievedValue);
            
            // 测试超长值
            string tooLongValue = new string('a', 257);
            Assert.False(_client.Set(key, tooLongValue));
        }

        [Fact]
        public void MaxConcurrentConnections()
        {
            const int maxConnections = 1000;
            var clients = new List<MemoryKVClient>();
            
            // 创建最大数量的连接
            for (int i = 0; i < maxConnections; i++)
            {
                var client = new MemoryKVClient();
                Assert.True(client.Initialize(ServiceName));
                clients.Add(client);
            }
            
            // 尝试创建额外连接
            using (var extraClient = new MemoryKVClient())
            {
                Assert.False(extraClient.Initialize(ServiceName));
            }
            
            // 清理
            foreach (var client in clients)
            {
                client.Dispose();
            }
        }

        [Fact]
        public void MemoryUsageLimit()
        {
            const int maxMemoryMB = 100; // 100MB 内存限制
            const int valueSize = 1024 * 1024; // 1MB 值大小
            
            // 尝试写入超过内存限制的数据
            string largeValue = new string('a', valueSize);
            for (int i = 0; i < maxMemoryMB + 1; i++)
            {
                string key = $"key_{i}";
                if (i < maxMemoryMB)
                {
                    Assert.True(_client.Set(key, largeValue));
                }
                else
                {
                    Assert.False(_client.Set(key, largeValue));
                }
            }
        }

        [Fact]
        public void EmptyKeyAndValue()
        {
            // 测试空键
            Assert.False(_client.Set("", "value"));
            
            // 测试空值
            Assert.True(_client.Set("key", ""));
            
            Assert.True(_client.Get("key", out string retrievedValue));
            Assert.Equal("", retrievedValue);
        }

        [Fact]
        public void SpecialCharacters()
        {
            // 测试包含特殊字符的键和值
            string specialKey = "key!@#$%^&*()";
            string specialValue = "value\n\t\r\0";
            
            Assert.True(_client.Set(specialKey, specialValue));
            
            Assert.True(_client.Get(specialKey, out string retrievedValue));
            Assert.Equal(specialValue, retrievedValue);
        }

        [Fact]
        public void BufferSize()
        {
            string key = "test_key";
            string value = "test_value";
            Assert.True(_client.Set(key, value));
            
            // 测试缓冲区不足
            Assert.False(_client.Get(key, out string retrievedValue, 5));
            
            // 测试缓冲区刚好
            Assert.True(_client.Get(key, out retrievedValue, value.Length + 1));
            Assert.Equal(value, retrievedValue);
        }
    }
} 