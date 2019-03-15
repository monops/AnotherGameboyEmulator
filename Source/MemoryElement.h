#pragma once

#include "Types.h"

class IMemoryElement
{
public:
	virtual ~IMemoryElement() = default;
	virtual uint8& ReadMemory(uint16 address) = 0;
	virtual void WriteMemory(uint16 address, uint8 value) = 0;

};