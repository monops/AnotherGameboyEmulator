#pragma once

#include "Types.h"

namespace BinaryOps
{
	template<typename TYPE>
	static inline bool GetBit(uint8 bit, TYPE value)
	{
		TYPE mask = 1 << bit;
		return !!(value & mask);
	}

	template<typename TYPE>
	static inline TYPE SetBit(uint8 bit, TYPE value, bool bitVal)
	{
		TYPE mask = 1 << bit;
		return bitVal ? value | mask : value & ~mask;
	}

	static inline uint8 SetMask(uint8 mask, uint8 oldValue, uint8 newValue)
	{
		return (oldValue & ~mask) | (newValue & mask);
	}
}