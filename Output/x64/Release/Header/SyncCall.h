#pragma once
#include <Windows.h>

/**
 * \brief call x statement in mutex
 * \param x please make sure x doesn't contain a return statement
 */
#define SYNC_CALL(x) \
{\
WaitForSingleObject(hMutex, INFINITE);\
\
try {\
    x;\
}\
catch (...) {\
    ReleaseMutex(hMutex);\
    throw;\
}\
ReleaseMutex(hMutex);\
}