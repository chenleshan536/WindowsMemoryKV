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
        static bool ParseCommand(string input, MemoryKV kv, string dbName)
        {
            // Split the input into parts based on spaces
            string[] parts = input.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);

            if (parts.Length == 0)
            {
                Console.WriteLine("Invalid command.");
                return false;
            }

            string command = parts[0].ToLower();  // The first word is the command
            string valuePrefix = "value";
            int startIndex = 1;
            int count = 10;
            int sleepInterval = 10;

            if(parts.Length >= 2)
                valuePrefix = parts[1];
            if (parts.Length >= 3)
                int.TryParse(parts[2], out startIndex);
            if (parts.Length >= 4)
                int.TryParse(parts[3], out count);
            if (parts.Length >= 5)
                int.TryParse(parts[4], out sleepInterval);

            switch (command)
            {
                case "put":
                    TestPut(kv, valuePrefix, startIndex, count, sleepInterval);
                    break;

                case "get":
                    TestGet(kv, valuePrefix, startIndex, count, sleepInterval);
                    break;

                case "exit":
                    Console.WriteLine("Exiting the program.");
                    return true;

                case "starthost":
                    MemoryKVHostServer.Run(dbName);
                    break;

                case "stophost":
                    MemoryKVHostServer.Stop();
                    break;

                default:
                    Console.WriteLine("Unknown command.");
                    break;
            }

            return false;
        }

        private static void TestGet(MemoryKV kv, string valuePrefix, int startIndex, int count, int sleepInterval)
        {
            Stopwatch sw = Stopwatch.StartNew();
            for (int i = 0; i < count; i++)
            {
                var val = kv.Get("key" + i);
            }
            sw.Stop();
            Console.WriteLine($"get done ,using {sw.Elapsed}");
        }

        private static void TestPut(MemoryKV kv, string valuePrefix, int startIndex, int count, int sleepInterval)
        {
            Stopwatch sw = Stopwatch.StartNew();
            for (int i = startIndex; i < count; i++)
            {
                kv.Put("key" + i, "value" + i);
            }
            sw.Stop();
            Console.WriteLine($"put done ,using {sw.Elapsed}");
        }

        static void Main(string[] args)
        {
            if (args.Length != 1)
            {
                Console.WriteLine("Usage wrong.\n");
                return;
            }

            var dbName = args[0];
            MemoryKV kv = new MemoryKV("csharp_client");
            kv.Open(dbName);

            while (true)
            {
                Console.WriteLine("please input command. examples:");
                Console.WriteLine("put value 1 1000 10");
                Console.WriteLine("get value 1 1000 0");
                Console.WriteLine("exit");
                Console.WriteLine("starthost");
                Console.WriteLine("stophost");

                var input = Console.ReadLine();
                if (ParseCommand(input, kv, dbName))
                    break;
            }

            Console.WriteLine("test done");
        }
    }
}
