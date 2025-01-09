del Output\. /s /q

msbuild WindowsMemoryKV.sln /t:Clean /p:Configuration=Debug /p:Platform=x86
msbuild WindowsMemoryKV.sln /p:Configuration=Debug /p:Platform=x86

msbuild WindowsMemoryKV.sln /t:Clean /p:Configuration=Release /p:Platform=x86
msbuild WindowsMemoryKV.sln /p:Configuration=Release /p:Platform=x86

msbuild WindowsMemoryKV.sln /t:Clean /p:Configuration=Debug /p:Platform=x64
msbuild WindowsMemoryKV.sln /p:Configuration=Debug /p:Platform=x64

msbuild WindowsMemoryKV.sln /t:Clean /p:Configuration=Release /p:Platform=x64
msbuild WindowsMemoryKV.sln /p:Configuration=Release /p:Platform=x64