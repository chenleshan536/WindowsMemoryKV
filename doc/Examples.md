# Client example
## C# example
```
using namespace MemoryKVLib.Net;

MemoryKV kv("my_client_name");
kv.Put("key1", "value1");
...


MemoryKV kv("my_client_name");
var result = kv.Get("key1");
...
kv.Remove("key1");

```

## C++ example
```

MemoryKV kv("my_client_name");
kv.Put("key1", "value1");
...


MemoryKV kv("my_client_name");
var result = kv.Get("key1");
...
kv.Remove("key1");
```

# Host server example
If you need a separate process to host the memory data when your client instance is gone, then you need to call this API.

## start a host server
    MemoryKVLib.Net.MemoryKVHostServer.Run("host_name"); //C# client
    MemoryKVHostServer::Run(L"host_name"); //C++ client

## stop the host server
    MemoryKVLib.Net.MemoryKVHostServer.Stop(); //C# client
    MemoryKVHostServer::Stop(); //C++ client
