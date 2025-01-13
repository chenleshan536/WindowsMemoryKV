using System;
using System.Runtime.InteropServices;

namespace MemoryKVLib.Net
{
    public class MemoryKV : IDisposable
    {
        private IntPtr _manager;
        
        public MemoryKV(string name)
        {
            _manager = MemoryKVNativeCall.MMFManager_create(name);
        }

        public void Open(string dbname)
        {
            MemoryKVNativeCall.MMFManager_open(_manager, dbname, ConfigOptions.Default);
        }

        public void Open(string dbname, ConfigOptions options)
        {
            MemoryKVNativeCall.MMFManager_open(_manager, dbname, options);
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