#include "MemoryKV.h"
#include "MemoryKVHostServer.h"

// C-style interface for C# to call
extern "C" __declspec(dllexport) MemoryKV* MMFManager_create(const wchar_t* clientName) {
    return new MemoryKV(clientName);
}

extern "C" __declspec(dllexport) void MMFManager_connect(MemoryKV* manager, const wchar_t* dbName, ConfigOptions options) {
    manager->OpenOrCreate(dbName, options);
}


extern "C" __declspec(dllexport) void MMFManager_destroy(MemoryKV* manager) {
        delete manager;
    }

extern "C" __declspec(dllexport) void MMFManager_put(MemoryKV* manager, const wchar_t* key, const wchar_t* value) {
        manager->Put(key, value);
    }

extern "C" __declspec(dllexport) const wchar_t* MMFManager_get(MemoryKV* manager, const wchar_t* key) {
        return manager->Get(key);
    }

extern "C" __declspec(dllexport) void MMFManager_remove(MemoryKV* manager, const wchar_t* key) {
        manager->Remove(key);
    }

extern "C" __declspec(dllexport) bool MemoryKvHost_startdefault(const wchar_t* clientName) {
    return MemoryKVHostServer::Run(clientName);
}

extern "C" __declspec(dllexport) bool MemoryKvHost_start(const wchar_t* clientName, ConfigOptions options, int refreshInterval) {
    return MemoryKVHostServer::Run(clientName, options, refreshInterval);
}

extern "C" __declspec(dllexport) bool MemoryKvHost_stop() {
    return MemoryKVHostServer::Stop();
}