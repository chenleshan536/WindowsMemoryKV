
Below test is done on a Dell Precision 5550 machine, with .NET6/x64/Release environment

## 10000 values randome write
1. WindowsMemoryKV, 0.0466136 sec
1. RocksDB, 0.0876445
1. LevelDB, 0.0281452
1. SQLite, 55.341000

## 10000 values read
1. WindowsMemoryKV, 0.0203119 sec
1. RocksDB, 0.0100719
1. LevelDB, 0.0138889
1. SQLite, 11.976000


## 10000 existed values update
1. WindowsMemoryKV, 0.0249142 sec
1. RocksDB, 0.0770493
1. LevelDB, 0.0266278
1. SQLite, 70.986000