#pragma once

#include "Types.h"
#include "Constants.h"
#include "MemoryElement.h"

#define JOYPAD_NONE             0

#define JOYPAD_INPUT_DOWN       1 << 3
#define JOYPAD_INPUT_UP         1 << 2
#define JOYPAD_INPUT_LEFT       1 << 1
#define JOYPAD_INPUT_RIGHT      1 << 0

#define JOYPAD_BUTTONS_START    1 << 3
#define JOYPAD_BUTTONS_SELECT   1 << 2
#define JOYPAD_BUTTONS_B        1 << 1
#define JOYPAD_BUTTONS_A        1 << 0

class GBInput : public IMemoryElement
{
public:
	GBInput(class GameBoyCPU* InCPU):
		m_CPU(InCPU)
	{}

	void Update();
	virtual uint8& ReadMemory(uint16 address) override;
	virtual void WriteMemory(uint16 address, uint8 Value) override;

private:
	GameBoyCPU* m_CPU = nullptr;
	uint8 m_SelectColumn = 0xff; //all buttons depressed
	uint8 m_Buttons = 0xFF;
	uint8 m_Joypad = 0xFF;

	uint8 m_CurrentRetVal = 0;
	uint8 m_FromSDL = 0;
};