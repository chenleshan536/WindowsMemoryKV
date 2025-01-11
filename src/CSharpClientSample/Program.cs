using System;
using System.Collections.Generic;
using System.Diagnostics;
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
            if (args.Length == 0)
            {
                Console.WriteLine("Usage wrong.\n");
                return;
            }
            string mode = args[0];
            int count = 0;
            int.TryParse(args[1], out count);

            if (mode == "put")
            {
                MemoryKV kv = new MemoryKV("csharp_client_put");
                kv.Connect("systemstate");
                Stopwatch sw = Stopwatch.StartNew();
                for (int i = 0; i < count; i++)
                {
                    kv.Put("key"+i, "value"+i);
                }
                sw.Stop();
                Console.WriteLine($"put done ,using {sw.Elapsed}");
            }
            else if (mode == "get")
            {
                MemoryKV kv = new MemoryKV("csharp_client_get"); 
                kv.Connect("systemstate");
                Stopwatch sw = Stopwatch.StartNew();
                for (int i = 0; i < count; i++)
                {
                    var val = kv.Get("key" + i);
                }
                sw.Stop();
                Console.WriteLine($"get done ,using {sw.Elapsed}");

            }
            
        }
    }
}
