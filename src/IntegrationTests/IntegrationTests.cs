using Xunit;
using System;
using System.Threading.Tasks;
using System.Diagnostics;
using MemoryKVLib.Net;
using CSharpClientSample;

namespace IntegrationTests
{
    public class IntegrationTests : IDisposable
    {
        private readonly Process _serviceProcess;
        private readonly string _serviceName = "test_integration_kv";
        private const int KeyLength = 64;
        private const int ValueLength = 256;
        private const int MMFCount = 100;
        private const int BlockPerMMF = 1000;

        public IntegrationTests()
        {
            // 启动服务进程
            _serviceProcess = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = "WindowsMemoryKVService.exe",
                    Arguments = $"--name {_serviceName} --key_length {KeyLength} --value_length {ValueLength} --mmf_count {MMFCount} --block_per_mmf {BlockPerMMF}",
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    CreateNoWindow = true
                }
            };
            _serviceProcess.Start();
            
            // 等待服务启动
            Task.Delay(1000).Wait();
        }

        public void Dispose()
        {
            // 停止服务进程
            if (!_serviceProcess.HasExited)
            {
                _serviceProcess.Kill();
                _serviceProcess.WaitForExit();
            }
            _serviceProcess.Dispose();
        }

        [Fact]
        public async Task ClientServerInteraction()
        {
            // 测试 C# 客户端与服务端的交互
            using var client = new MemoryKVClient();
            Assert.True(client.Initialize(_serviceName));

            const string key = "test_key";
            const string value = "test_value";

            // 测试写入
            Assert.True(client.Set(key, value));

            // 测试读取
            Assert.True(client.Get(key, out string result));
            Assert.Equal(value, result);

            // 测试删除
            Assert.True(client.Delete(key));
            Assert.False(client.Get(key, out _));
        }

        [Fact]
        public async Task CrossLanguageInterop()
        {
            // 测试 C++ 和 C# 客户端之间的互操作性
            using var csharpClient = new MemoryKVClient();
            using var cppClient = new MemoryKV();
            
            Assert.True(csharpClient.Initialize(_serviceName));
            Assert.True(cppClient.Initialize(_serviceName, KeyLength, ValueLength, MMFCount, BlockPerMMF));

            const string key = "cross_lang_key";
            const string value = "cross_lang_value";

            // C# 客户端写入
            Assert.True(csharpClient.Set(key, value));

            // C++ 客户端读取
            char[] buffer = new char[ValueLength];
            Assert.True(cppClient.Get(key, buffer, ValueLength));
            Assert.Equal(value, new string(buffer).TrimEnd('\0'));

            // C++ 客户端写入新值
            const string newValue = "new_cross_lang_value";
            Assert.True(cppClient.Set(key, newValue));

            // C# 客户端读取
            Assert.True(csharpClient.Get(key, out string result));
            Assert.Equal(newValue, result);
        }

        [Fact]
        public async Task ConcurrentClients()
        {
            const int clientCount = 4;
            const int operationsPerClient = 1000;
            var tasks = new Task[clientCount];

            for (int i = 0; i < clientCount; i++)
            {
                int clientId = i;
                tasks[i] = Task.Run(async () =>
                {
                    using var client = new MemoryKVClient();
                    Assert.True(client.Initialize(_serviceName));

                    for (int j = 0; j < operationsPerClient; j++)
                    {
                        string key = $"key_{clientId}_{j}";
                        string value = $"value_{clientId}_{j}";

                        Assert.True(client.Set(key, value));
                        Assert.True(client.Get(key, out string result));
                        Assert.Equal(value, result);
                        Assert.True(client.Delete(key));
                    }
                });
            }

            await Task.WhenAll(tasks);
        }

        [Fact]
        public async Task ServiceRecovery()
        {
            using var client = new MemoryKVClient();
            Assert.True(client.Initialize(_serviceName));

            const string key = "recovery_key";
            const string value = "recovery_value";

            // 写入数据
            Assert.True(client.Set(key, value));

            // 模拟服务崩溃
            _serviceProcess.Kill();
            _serviceProcess.WaitForExit();

            // 重启服务
            _serviceProcess.Start();
            await Task.Delay(1000);

            // 验证数据恢复
            Assert.True(client.Get(key, out string result));
            Assert.Equal(value, result);
        }
    }
} 