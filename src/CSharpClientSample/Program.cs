using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MemoryKVLib.Net;

namespace CSharpClientSample
{
    internal class Program
    {
        
        static void Main(string[] args)
        {
            MemoryKV kv = new MemoryKV("CsharpClient", MemoryKV.DefaultConfigOptions);
            kv.Put("key3", "value3_10293");
            Console.WriteLine("put key3");
            Console.WriteLine("get key2="+ kv.Get("key2"));
            Console.ReadLine();
        }
    }
}
