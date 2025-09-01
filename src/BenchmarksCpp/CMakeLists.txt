cmake_minimum_required(VERSION 3.10)
project(BenchmarksCpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 Google Benchmark
include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(benchmark)

# 添加 MemoryKVLib
add_subdirectory(../MemoryKVLib ${CMAKE_BINARY_DIR}/MemoryKVLib)

# 创建性能测试可执行文件
add_executable(BenchmarksCpp
    MemoryKVBenchmarks.cpp
)

# 链接库
target_link_libraries(BenchmarksCpp
    PRIVATE
    benchmark::benchmark
    MemoryKVLib
) 