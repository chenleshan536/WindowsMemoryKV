using System;
using System.Runtime.InteropServices;

namespace MemoryKVLib.Net
{
    public class MemoryKV : IDisposable
    {
        private IntPtr _manager;

        public static ConfigOptions DefaultConfigOptions = new ConfigOptions()
        {
            MaxKeySize = 64, MaxValueSize = 256, MaxBlocksPerMmf = 1000, MaxMmfCount = 100
        };
        public MemoryKV(string name)
        {
            _manager = MemoryKVNativeCall.MMFManager_createdefault(name);
        }

        public MemoryKV(string name, ConfigOptions options)
        {
            _manager = MemoryKVNativeCall.MMFManager_create(name, options);
        }

        public bool Put(string key, string value)
        {
            return MemoryKVNativeCall.MMFManager_put(_manager, key, value);
        }

        public string Get(string key)
        {
            IntPtr ptr = MemoryKVNativeCall.MMFManager_get(_manager, key);
            string greeting = Marshal.PtrToStringUni(ptr);
            return greeting;
        }

        public void Remove(string key)
        {
            MemoryKVNativeCall.MMFManager_remove(_manager, key);
        }

        public void Dispose()
        {
            MemoryKVNativeCall.MMFManager_destroy(_manager);
        }
    }
}