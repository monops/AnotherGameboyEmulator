#include "MemoryModel.h"

//Lots of this code from "GameLad"
//https://github.com/Dooskington/GameLad

#define EnableRAM   0x0A
#define ROMBankMode 0x00
#define RAMBankMode 0x01

void GameBoyMemory::RegisterElementRange(uint16 From, uint16 To, IMemoryElement* Pointer)
{
	for (int32 i = From; i <= To; ++i)
	{
		m_MemoryMap[i] = Pointer;
	}
}

void GameBoyMemory::RegisterElement(uint16 Address, IMemoryElement* Pointer)
{
	m_MemoryMap[Address] = Pointer;
}

uint8& GameBoyMemory::Read(uint16 address )
{
	return m_MemoryMap[address]->ReadMemory(address);
}

void GameBoyMemory::Write(uint16 address, uint8 Value)
{
	m_MemoryMap[address]->WriteMemory(address, Value);
}

uint8& GameBoyMemory::ReadMemory(uint16 address)
{
	if (address >= 0xC000 && address <= 0xDFFF)
	{
		//RAM
		return m_InternalRAM[address - 0xC000];
	}
	else if (address >= 0xE000 && address <= 0xFDFF)
	{
		//RAM echo
		return m_InternalRAM[address - 0xE000];
	}
	else if (address >= 0xFF80 && address <= 0xFFFE)
	{
		return m_HInternalRAM[address - 0xFF80];
	}
	else if (address == 0xFFFF)
	{
		return m_InterruptEnabled;
	}
	else if (address == 0xFF0F)
	{
		return m_InterruptFlags;
	}
	else if (address == 0xFF50)
	{
		return m_IsBooting;
	}
	else
	{
		static uint8 ZeroVal = 0x00;
		return ZeroVal;
	}
}

void GameBoyMemory::WriteMemory(uint16 address, uint8 Value)
{
	if (address >= 0xC000 && address <= 0xDFFF)
	{
		//RAM
		m_InternalRAM[address - 0xC000] = Value;
	}
	else if (address >= 0xE000 && address <= 0xFDFF)
	{
		//RAM echo
		m_InternalRAM[address - 0xE000] = Value;
	}
	else if (address >= 0xFF80 && address <= 0xFFFE)
	{
		m_HInternalRAM[address - 0xFF80] = Value;
	}
	else if (address == 0xFFFF)
	{
		m_InterruptEnabled = Value;
	}
	else if (address == 0xFF0F)
	{
		m_InterruptFlags = Value;
	}
	else if (address == 0xFF50)
	{
		m_IsBooting = Value;
	}
}


uint8& MEM_ROMOnly::ReadMemory(uint16 address)
{
	return m_ROM[address];
}

void MEM_ROMOnly::WriteMemory(uint16 address, uint8 Value)
{
	
}


MEM_MBC1::MEM_MBC1(uint8* pROM, uint8* pRAM) :
	IROMMemoryModel(pROM, pRAM),
	m_ROMBankLower(0x01),
	m_ROMRAMBankUpper(0x00),
	m_ROMRAMMode(0x0)
{
}

uint8& MEM_MBC1::ReadMemory(uint16 address)
{
	if (address <= 0x3FFF)
	{
		/*
		0000-3FFF - ROM Bank 00 (Read Only)
		This area always contains the first 16KBytes of the cartridge ROM.
		*/
		return m_ROM[address];
	}
	else if (address <= 0x7FFF)
	{
		/*
		4000-7FFF - ROM Bank 01-7F (Read Only)
		This area may contain any of the further 16KByte banks of the ROM, allowing to address up to 125 ROM
		Banks (almost 2MByte). As described below, bank numbers 20h, 40h, and 60h cannot be used, resulting
		in the odd amount of 125 banks.
		*/
		uint8 targetBank = m_ROMBankLower;
		if (m_ROMRAMMode == ROMBankMode)
		{
			// The upper bank values are only available in ROM Bank Mode
			targetBank |= (m_ROMRAMBankUpper << 4);
		}

		unsigned int target = (address - 0x4000);
		target += (0x4000 * targetBank);
		return m_ROM[target];
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		/*
		A000-BFFF - RAM Bank 00-03, if any (Read/Write)
		This area is used to address external RAM in the cartridge (if any). External RAM is often battery
		buffered, allowing to store game positions or high score tables, even if the gameboy is turned off,
		or if the cartridge is removed from the gameboy. Available RAM sizes are: 2KByte (at A000-A7FF),
		8KByte (at A000-BFFF), and 32KByte (in form of four 8K banks at A000-BFFF).
		*/
		static uint8 FF = 0xFF;
		if (!m_IsRAMEnabled)
		{
			// RAM disabled
			return FF;
		}

		if (m_RAM == nullptr)
		{
			// RAM not initialized
			return FF;
		}

		// In ROM Mode, only bank 0x00 is available
		unsigned int target = address - 0xA000;
		if (m_ROMRAMMode == RAMBankMode)
		{
			// Offset based on the bank number
			target += (0x2000 * m_ROMRAMBankUpper);
		}

		return m_RAM[target];
	}

	static uint8 Zero = 0;
	return Zero;
}

void MEM_MBC1::WriteMemory(uint16 address, uint8 Value)
{
	if (address <= 0x1FFF)
	{
		/*
		0000-1FFF - RAM Enable (Write Only)
		Before external RAM can be read or written, it must be enabled by writing to this address space.
		It is recommended to disable external RAM after accessing it, in order to protect its contents from
		damage during power down of the gameboy. Usually the following values are used:
		00h  Disable RAM (default)
		0Ah  Enable RAM
		Practically any value with 0Ah in the lower 4 bits enables RAM, and any other value disables RAM.
		*/
		m_IsRAMEnabled = ((Value & EnableRAM) == EnableRAM);
		return;
	}
	else if (address <= 0x3FFF)
	{
		/*
		2000-3FFF - ROM Bank Number (Write Only)
		Writing to this address space selects the lower 5 bits of the ROM Bank Number (in range 01-1Fh).
		When 00h is written, the MBC translates that to bank 01h also. That doesn't harm so far, because ROM
		Bank 00h can be always directly accessed by reading from 0000-3FFF.
		But (when using the register below to specify the upper ROM Bank bits), the same happens for Bank
		20h, 40h, and 60h. Any attempt to address these ROM Banks will select Bank 21h, 41h, and 61h instead.
		*/
		m_ROMBankLower = Value & 0x1F;
		if (m_ROMBankLower == 0x00)
		{
			m_ROMBankLower = 0x01;
		}

		return;
	}
	else if (address <= 0x5FFF)
	{
		/*
		4000-5FFF - RAM Bank Number - or - Upper Bits of ROM Bank Number (Write Only) This 2bit register
		can be used to select a RAM Bank in range from 00-03h, or to specify the upper two bits (Bit 5-6) of
		the ROM Bank number, depending on the current ROM/RAM Mode. (See below.)
		*/

		m_ROMRAMBankUpper = Value & 0x03;
		return;
	}
	else if (address <= 0x7FFF)
	{
		/*
		6000-7FFF - ROM/RAM Mode Select (Write Only)
		This 1bit Register selects whether the two bits of the above register should be used as upper two
		bits of the ROM Bank, or as RAM Bank Number.
		00h = ROM Banking Mode (up to 8KByte RAM, 2MByte ROM) (default)
		01h = RAM Banking Mode (up to 32KByte RAM, 512KByte ROM)
		The program may freely switch between both modes, the only limitiation is that only RAM Bank 00h
		can be used during Mode 0, and only ROM Banks 00-1Fh can be used during Mode 1.
		*/
		m_ROMRAMMode = Value & 0x01;
		return;
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		/*
		A000-BFFF - RAM Bank 00-03, if any (Read/Write)
		This area is used to address external RAM in the cartridge (if any). External RAM is often battery
		buffered, allowing to store game positions or high score tables, even if the gameboy is turned off,
		or if the cartridge is removed from the gameboy. Available RAM sizes are: 2KByte (at A000-A7FF),
		8KByte (at A000-BFFF), and 32KByte (in form of four 8K banks at A000-BFFF).
		*/

		if (!m_IsRAMEnabled)
		{
			// RAM disabled
			return;
		}

		if (m_RAM == nullptr)
		{
			// RAM not initialized
			return;
		}

		// In ROM Mode, only bank 0x00 is available
		unsigned int target = address - 0xA000;
		if (m_ROMRAMMode == RAMBankMode)
		{
			// Offset based on the bank number
			target += (0x2000 * m_ROMRAMBankUpper);
		}

		m_RAM[target] = Value;
		return;
	}

}

MEM_MBC2::MEM_MBC2(uint8* pROM, uint8* pRAM) :
	IROMMemoryModel(pROM, pRAM),
	m_ROMBank(0x01)
{
}

uint8& MEM_MBC2::ReadMemory(uint16 address)
{
	if (address <= 0x3FFF)
	{
		/*
		0000-3FFF - ROM Bank 00 (Read Only)
		This area always contains the first 16KBytes of the cartridge ROM.
		*/
		return m_ROM[address];
	}
	else if (address <= 0x7FFF)
	{
		/*
		4000-7FFF - ROM Bank 01-7F (Read Only)
		This area may contain any of the further 16KByte banks of the ROM, allowing to address up to 16 ROM
		Banks (almost 256KByte).
		*/
		unsigned int target = (address - 0x4000);
		target += (0x4000 * m_ROMBank);
		return m_ROM[target];
	}
	else if (address >= 0xA000 && address <= 0xA1FF)
	{
		/*
		A000-A1FF - 512x4bits RAM, built-in into the MBC2 chip (Read/Write)
		The MBC2 doesn't support external RAM, instead it includes 512x4 bits of built-in RAM (in the MBC2
		chip itself). It still requires an external battery to save data during power-off though.
		As the data consists of 4bit values, only the lower 4 bits of the "bytes" in this memory area are used.
		*/
		if (!m_IsRAMEnabled)
		{
			static uint8 FF = 0xFF;
			return FF;
		}

		return m_RAM[address - 0xA000];
	}

	static uint8 Zero = 0;
	return Zero;
}

void MEM_MBC2::WriteMemory(uint16 address, uint8 Value)
{
	if (address <= 0x1FFF)
	{
		/*
		0000-1FFF - RAM Enable (Write Only)
		The least significant bit of the upper address byte must be zero to enable/disable cart RAM. For
		example the following addresses can be used to enable/disable cart RAM: 0000-00FF, 0200-02FF,
		0400-04FF, ..., 1E00-1EFF.
		The suggested address range to use for MBC2 ram enable/disable is 0000-00FF.
		*/
		if ((address & 0x0100) == 0x0000)
		{
			m_IsRAMEnabled = ((Value & EnableRAM) == EnableRAM);
			return;
		}
	}
	else if (address <= 0x3FFF)
	{
		/*
		2000-3FFF - ROM Bank Number (Write Only)
		Writing a value (XXXXBBBB - X = Don't cares, B = bank select bits) into 2000-3FFF area will select
		an appropriate ROM bank at 4000-7FFF.

		The least significant bit of the upper address byte must be one to select a ROM bank. For example
		the following addresses can be used to select a ROM bank: 2100-21FF, 2300-23FF, 2500-25FF, ...,
		3F00-3FFF.
		The suggested address range to use for MBC2 rom bank selection is 2100-21FF.
		*/
		if ((address & 0x0100) == 0x0000)
		{
			m_ROMBank = (Value & 0x0F);
			return;
		}
	}
	else if (address >= 0xA000 && address <= 0xA1FF)
	{
		/*
		A000-A1FF - 512x4bits RAM, built-in into the MBC2 chip (Read/Write)
		The MBC2 doesn't support external RAM, instead it includes 512x4 bits of built-in RAM (in the MBC2
		chip itself). It still requires an external battery to save data during power-off though.
		As the data consists of 4bit values, only the lower 4 bits of the "bytes" in this memory area are used.
		*/
		if (!m_IsRAMEnabled)
		{
			return;
		}

		m_RAM[address - 0xA000] = (Value & 0x0F);
		return;
	}

	return;
}

MEM_MBC3::MEM_MBC3(uint8* pROM, uint8* pRAM) :
	IROMMemoryModel(pROM, pRAM),
	m_ROMBank(0x01),
	m_RAMBank(0x00)
{
	memset(m_RTCRegisters, 0x00, ARRAYSIZE(m_RTCRegisters));
}


uint8& MEM_MBC3::ReadMemory(uint16 address)
{
	if (address <= 0x3FFF)
	{
		/*
		0000-3FFF - ROM Bank 00 (Read Only)
		Same as for MBC1.
		*/
		return m_ROM[address];
	}
	else if (address <= 0x7FFF)
	{
		/*
		4000-7FFF - ROM Bank 01-7F (Read Only)
		Same as for MBC1, except that accessing banks 20h, 40h, and 60h is supported now.
		*/
		unsigned int target = (address - 0x4000);
		target += (0x4000 * m_ROMBank);
		return m_ROM[target];
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		/*
		A000-BFFF - RAM Bank 00-03, if any (Read/Write)
		A000-BFFF - RTC Register 08-0C (Read/Write)
		Depending on the current Bank Number/RTC Register selection (see below), this memory space is used
		to access an 8KByte external RAM Bank, or a single RTC Register.
		*/

		static uint8 FF = 0xFF;
		if (!m_IsRAMEnabled)
		{
			return FF;
		}

		if (m_RAM == nullptr)
		{
			return FF;
		}

		if (m_RAMBank <= 0x03)
		{
			unsigned int target = address - 0xA000;
			// Offset based on the bank number
			target += (0x2000 * m_RAMBank);
			return m_RAM[target];
		}
		else if (m_RAMBank >= 0x08 && m_RAMBank <= 0x0C)
		{
			return m_RTCRegisters[m_RAMBank - 0x08];
		}
	}

	static uint8 Zero = 0;
	return Zero;
}

void MEM_MBC3::WriteMemory(uint16 address, uint8 Value)
{
	if (address <= 0x1FFF)
	{
		/*
		0000-1FFF - RAM and Timer Enable (Write Only)
		Mostly the same as for MBC1, a value of 0Ah will enable reading and writing to external RAM - and
		to the RTC Registers! A value of 00h will disable either.
		*/
		m_IsRAMEnabled = ((Value & EnableRAM) == EnableRAM);
		return;
	}
	else if (address <= 0x3FFF)
	{
		/*
		2000-3FFF - ROM Bank Number (Write Only)
		Same as for MBC1, except that the whole 7 bits of the RAM Bank Number are written directly to this
		address. As for the MBC1, writing a value of 00h, will select Bank 01h instead. All other values
		01-7Fh select the corresponding ROM Banks.
		*/
		m_ROMBank = (Value & 0x7F);
		if (m_ROMBank == 0x00)
		{
			m_ROMBank = 0x01;
		}

		return;
	}
	else if (address <= 0x5FFF)
	{
		/*
		4000-5FFF - RAM Bank Number - or - RTC Register Select (Write Only)
		As for the MBC1s RAM Banking Mode, writing a value in range for 00h-03h maps the corresponding
		external RAM Bank (if any) into memory at A000-BFFF.
		When writing a value of 08h-0Ch, this will map the corresponding RTC register into memory at
		A000-BFFF. That register could then be read/written by accessing any address in that area,
		typically that is done by using address A000.
		*/
		m_RAMBank = Value;
		return;
	}
	else if (address <= 0x7FFF)
	{
		/*
		6000-7FFF - Latch Clock Data (Write Only)
		When writing 00h, and then 01h to this register, the current time becomes latched into the RTC
		registers. The latched data will not change until it becomes latched again, by repeating the
		write 00h->01h procedure.
		This is supposed for <reading> from the RTC registers. It is proof to read the latched (frozen)
		time from the RTC registers, while the clock itself continues to tick in background.
		*/

		// TODO: Look at this for ideas:
		// https://github.com/creker/Cookieboy/blob/a476f8ec5baffd176ee1572885edb7f41b241571/CookieboyMBC3.h
		return;
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		/*
		A000-BFFF - RAM Bank 00-03, if any (Read/Write)
		A000-BFFF - RTC Register 08-0C (Read/Write)
		Depending on the current Bank Number/RTC Register selection (see below), this memory space is used
		to access an 8KByte external RAM Bank, or a single RTC Register.
		*/

		if (!m_IsRAMEnabled)
		{
			return;
		}

		if (m_RAM == nullptr)
		{
			return;
		}

		if (m_RAMBank <= 0x03)
		{
			unsigned int target = address - 0xA000;
			// Offset based on the bank number
			target += (0x2000 * m_RAMBank);
			m_RAM[target] = Value;
			return;
		}
		else if (m_RAMBank >= 0x08 && m_RAMBank <= 0x0C)
		{
			m_RTCRegisters[m_RAMBank - 0x08] = Value;
			return;
		}
	}

	return;
}