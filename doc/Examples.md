# Client example
## C# example
```
using namespace MemoryKVLib.Net;

MemoryKV kv("my_client_name");
kv.Open("mydomain1");
kv.Put("key1", "value1");
...


MemoryKV kv("my_client_name");
kv.Open("mydomain2");
var result = kv.Get("key1");
...
kv.Remove("key1");

```

## C++ example
```

MemoryKV kv("my_client_name");
kv.Open("mydomain1");
kv.Put("key1", "value1");
...


MemoryKV kv("my_client_name");
kv.Open("mydomain2");
var result = kv.Get("key1");
...
kv.Remove("key1");
```

# Host server example
If you need a separate process to host the memory data when your client instance is gone, then you need to call this API.

## C# example
```
    MemoryKVLib.Net.MemoryKVHostServer.Run("mydomain1"); // start a host server process that watch (hold) the memory data for db mydomain1
    MemoryKVLib.Net.MemoryKVHostServer.Run("mydomain2"); // add a watcher for db mydomain2
    MemoryKVLib.Net.MemoryKVHostServer.Stop("mydomain1"); //stop the watcher for db mydomain1
    MemoryKVLib.Net.MemoryKVHostServer.StopAll(); //stop all the watcher and exit the host server process
```

## C++ example
```
    MemoryKVHostServer::Run("mydomain1"); // start a host server process that watch (hold) the memory data for db mydomain1
    MemoryKVHostServer::Run("mydomain2"); // add a watcher for db mydomain2
    MemoryKVHostServer::Stop("mydomain1"); //stop the watcher for db mydomain1
    MemoryKVHostServer::StopAll(); //stop all the watcher and exit the host server process
```