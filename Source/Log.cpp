#include "Log.h"
#include <xstring>
#include <stdarg.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void Log::log(const char* format, ...)
{
#ifdef _DEBUG
	std::string stringWithReturn(format);
	stringWithReturn += L'\n';
	stringWithReturn = stringWithReturn;

	char buffer[10 * 1024];
	va_list argptr;
	va_start(argptr, format);
	vsprintf_s(buffer, 10 * 1024, stringWithReturn.c_str(), argptr);
	va_end(argptr);

	printf("%s", buffer);
	OutputDebugString(buffer);
#endif
}