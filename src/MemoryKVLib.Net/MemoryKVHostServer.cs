﻿namespace MemoryKVLib.Net
{
    public class MemoryKVHostServer
    {
        public static bool Run(string name)
        {
            return MemoryKVNativeCall.MemoryKvHost_startdefault(name);
        }

        public static bool Run(string name, ConfigOptions options, int refreshInterval)
        {
            return MemoryKVNativeCall.MemoryKvHost_start(name, options, refreshInterval);
        }

        public static bool Stop()
        {
            return MemoryKVNativeCall.MemoryKvHost_stop();
        }
    }
}