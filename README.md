# WindowsMemoryKV
Windows platform fast Memory-based KV storage library, supports C++ and  C# (.NET FWK, .NET  CORE)

# Feature/Requirement List
[feature list](https://github.com/chenleshan536/WindowsMemoryKV/blob/main/doc/feature%20list.md) document

# Usage Examples

## C# example
```
using namespace MemoryKVLib.Net;

MemoryKV kv("my_client_name");
kv.Put("key1", "value1");
...


MemoryKV kv("my_client_name");
var result = kv.Get("key1");
```

## C++ example
```

MemoryKV kv("my_client_name");
kv.Put("key1", "value1");
...


MemoryKV kv("my_client_name");
var result = kv.Get("key1");
```


# Release History

