#pragma once

#include "Types.h"

class Log
{
public:
	static void log(const char* format, ...);
};

#if DEBUG
#define GB_LOG(...) Log::log(__VA_ARGS__)
#else
#define GB_LOG(...)
#endif