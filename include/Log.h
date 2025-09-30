#ifndef LOG_HPP
#define LOG_HPP

//#define TIGER_DLT 1
#if TIGER_DLT
#include "servicelayer/Log.h"
#define LOG_TAG "Dispatcher"

#else
//https://stackoverflow.com/questions/98944/how-to-generate-a-newline-in-a-cpp-macro
#define LOGI(...) do { } while (0)
#endif

#endif // LOG_HPP