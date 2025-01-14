#pragma once
#include <Windows.h>

/**
 * \brief call x statement in mutex
 * \param x please make sure x doesn't contain a return statement
 */
#define SYNC_CALL(x) \
{\
WaitForSingleObject(m_hMutex, INFINITE);\
\
try {\
    x;\
}\
catch (...) {\
    ReleaseMutex(m_hMutex);\
    throw;\
}\
ReleaseMutex(m_hMutex);\
}