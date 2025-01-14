#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>

#define PIPE_NAME L"\\\\.\\pipe\\TaskPipe2"