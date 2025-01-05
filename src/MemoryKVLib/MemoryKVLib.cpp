#include "MemoryKV.h"

// C-style interface for C# to call
extern "C" __declspec(dllexport) MemoryKV* MMFManager_createdefault(const wchar_t* clientName) {
        return new MemoryKV(clientName);
    }

extern "C" __declspec(dllexport) MemoryKV* MMFManager_create(const wchar_t* clientName, ConfigOptions options) {
    return new MemoryKV(clientName, options);
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

