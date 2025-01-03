# Design Objectives and remaining tasks:

## 1 Put function
1. Client can know the put operation succeeds or not
1. Put key and value should follow the length limitation, otherwise reject the request and let client know
1. Update same key again should not create a new block, no matter which instance creates that key (which means, if the key is put by another client before, then it must be synched or searched before updating) -- done

## 2 Get function
1. query keys should return correct and consistent result -- done
1. query non-existing keys should return empty string, not crash -- done
1. if one key is put successfully from one instance, it should be visible for query from all instances immediatelly -- done

## 3 Remove function
1. Removed keys should not be queried from all instances
1. Removed data blocks should be reused by all instances
1. The new added key on the removed slots should be queried from all instances 

## 4 Server process
1. There is a way to host the data until it's finally released by the user
1. Needs separate class and separate API for host server process
### 4.1 Tasks
1. The new created data blocks should be kept as well

## 5 Performance
1. It should beat most of the competitors (RocksDB, LevelDB, SQLite, etc.)
1. The performance should not drop as more keys are added -- design and impl done, testing pending
### 5.1 Tasks
1. Use hash code not loop to query -- done
1. Hashmap stay up to date after other instance processing (Put/Get) -- done
1. Hashmap stay up to date after other instance processing (Remove)

## 6 Memory usage (resize problem)
1. There is no limitation on the number of clients and client processes.
1. There is no significant memory increase for the whole system when a new client is added
1. Take small size at beginning, and resize when needed - done
1. Don't copy data when resizing, extend data section - done
1. Shrink the size when no need -- no shrink

## 7 Data consistency among multiple instances (auto sync problem)
1. There is no inconsistent observable data between multiple instances, i.e., no matter client do Put/Get/Remove operation on any of the instance it should be the same result at all times.

## 8 Maintenance
1. No third-party dependencies -- done
1. Split headerblock and datablock management -- done
1. Split C++ header files -- done
1. Unit test 

## 9 Logs
1. Simple file logger -- done
1. log client operation -- done
1. add client ID in logs -- done

## 10 API
1. C++ API: header, lib, and DLL -- done
1. C# API with same signature: -- done
1. C# supports both .NET FWK and .NET CORE -- done

## 11 Configuration
1. Configurable data sizes -- done

## 12 Testing
1. Performance benchmark testing 
1. Ablility to keep testing -- done
1. Long endurance, concurrent testing -- done

## 13 Misc
1. save current snapshot
1. View/Search/Show KV state