#pragma once
#include <memory>
#include <string>
#include <xstring>
#include <codecvt>
#include <unordered_map>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning(disable :4251)

#define ALIGN(val) __declspec(align(val))

using int32 = signed int;
using int64 = signed long long;

using uint32 = unsigned int;
using uint64 = unsigned long long;
using uint8 = unsigned char;
using int8 = signed char;
using uint16 = unsigned short;
using INT16 = signed short;


using JSizeT = size_t;

template<typename TYPE>
using JUniPtr = std::unique_ptr<TYPE>;

template<typename TYPE>
using JRefPtr = std::shared_ptr<TYPE>;

