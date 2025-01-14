using System;
using System.Runtime.InteropServices;

namespace MemoryKVLib.Net
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ConfigOptions
    {
        public int MaxKeySize;
        public int MaxValueSize;
        public int MaxBlocksPerMmf;
        public int MaxMmfCount;
        public int LogLevel;
        
        public ConfigOptions(int maxKeySize, int maxValueSize, int maxBlocksPerMmf, int maxMmfCount, int logLevel) : this()
        {
            MaxKeySize = maxKeySize;
            MaxValueSize = maxValueSize;
            MaxBlocksPerMmf = maxBlocksPerMmf;
            MaxMmfCount = maxMmfCount;
            LogLevel = logLevel;
        }

        public static ConfigOptions Default => new ConfigOptions(64, 256,1000, 100, 1);
        
    }

    internal class MemoryKVNativeCall
    {

        [DllImport("MemoryKVLib.dll", EntryPoint = "MMFManager_create", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr MMFManager_create(string clientName);

        [DllImport("MemoryKVLib.dll", EntryPoint = "MMFManager_open", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr MMFManager_open(IntPtr manager, string dbName, ConfigOptions options);


        [DllImport("MemoryKVLib.dll", EntryPoint = "MMFManager_destroy", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MMFManager_destroy(IntPtr manager);

        [DllImport("MemoryKVLib.dll", EntryPoint = "MMFManager_put", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool MMFManager_put(IntPtr manager, string key, string value);

        [DllImport("MemoryKVLib.dll", EntryPoint = "MMFManager_get", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        //[return: MarshalAs(UnmanagedType.LPStr)]
        public static extern IntPtr MMFManager_get(IntPtr manager, string key);

        [DllImport("MemoryKVLib.dll", EntryPoint = "MMFManager_remove", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MMFManager_remove(IntPtr manager, string key);

        [DllImport("MemoryKVLib.dll", EntryPoint = "MemoryKvHost_startdefault", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool MemoryKvHost_startdefault(string dbName);

        [DllImport("MemoryKVLib.dll", EntryPoint = "MemoryKvHost_start", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool MemoryKvHost_start(string dbName, ConfigOptions options, int refreshInterval);

        [DllImport("MemoryKVLib.dll", EntryPoint = "MemoryKvHost_stop", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool MemoryKvHost_stop(string dbName);

        [DllImport("MemoryKVLib.dll", EntryPoint = "MemoryKvHost_stopall", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool MemoryKvHost_stopall();
    }
}