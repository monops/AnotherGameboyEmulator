#include "Input.h"
#include "CPU.h"
#include "SDL.h"
#include "BinaryOps.h"

using namespace BinaryOps;

void GBInput::Update()
{
	SDL_PumpEvents();
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	uint8 nJoypad = JOYPAD_NONE;
	uint8 nButtons = JOYPAD_NONE;

	if (keys[SDL_SCANCODE_UP])
	{
		nJoypad |= JOYPAD_INPUT_UP;
	}

	if (keys[SDL_SCANCODE_LEFT])
	{
		nJoypad |= JOYPAD_INPUT_LEFT;
	}

	if (keys[SDL_SCANCODE_DOWN])
	{
		nJoypad |= JOYPAD_INPUT_DOWN;
	}

	if (keys[SDL_SCANCODE_RIGHT])
	{
		nJoypad |= JOYPAD_INPUT_RIGHT;
	}

	if (keys[SDL_SCANCODE_Z])
	{
		nButtons |= JOYPAD_BUTTONS_A;
	}

	if (keys[SDL_SCANCODE_X])
	{
		nButtons |= JOYPAD_BUTTONS_B;
	}

	if (keys[SDL_SCANCODE_RETURN])
	{
		nButtons |= JOYPAD_BUTTONS_START;
	}

	if (keys[SDL_SCANCODE_RSHIFT])
	{
		nButtons |= JOYPAD_BUTTONS_SELECT;
	}

	uint8 inputChanges = !GetBit(4, m_SelectColumn) ? (m_Joypad ^ nJoypad) : 0x00;
	uint8 buttonChanges = !GetBit(5, m_SelectColumn) ? (m_Buttons ^ nButtons) : 0x00;

	m_Joypad = nJoypad;
	m_Buttons = nButtons;

	// If either the input or buttons change and they were requested, trigger interrupt
	if ((m_CPU != nullptr) && ((inputChanges > 0x00) || (buttonChanges > 0x00)))
	{
		m_CPU->FireInterrupt(InterruptCodes::Joypad);
	}
}

uint8& GBInput::ReadMemory(uint16 address)
{
	static uint8 Zero = 0;
	uint8 input = 0x00;

	switch (address)
	{
	case MemRegisters::InputRegister:
		if (!GetBit(4, m_SelectColumn))
		{
			//Check Joypad
			input |= m_Joypad;
		}

		if (!GetBit(5, m_SelectColumn))
		{
			//check buttons
			input |= m_Buttons;
		}
		m_CurrentRetVal = ((m_SelectColumn | 0x0F) ^ input) & 0x3F;
		return m_CurrentRetVal;
		break;
	default:
		return Zero;
		break;
	}

	return Zero;
}

void GBInput::WriteMemory(uint16 address, uint8 Value)
{
	switch (address)
	{
	case MemRegisters::InputRegister:
		m_SelectColumn = Value & 0x30; //just bits 4&5
		break;
	default:
		break;
	}
}