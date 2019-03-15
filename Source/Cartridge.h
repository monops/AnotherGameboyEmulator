#pragma once

#include "Types.h"
#include "MemoryModel.h"
#include <string>

enum class CartrigeType : uint8
{
	ROMOnly = 0x00,
	ROM_MBC1 = 0x01,
	ROM_MBC1_RAM = 0x02,
	ROM_MBC1_RAM_BATTERY = 0x03,
	ROM_MBC2 = 0x05,
	ROM_MBC2_BATTERY = 0x06,
	ROM_RAM = 0x08,
	ROM_RAM_BATTERY = 0x09,
	ROM_MMM01 = 0x0B,
	ROM_MMM01_SDRAM = 0x0C,
	ROM_MMM01_SDRAM_BATTERY = 0x0D,
	ROM_MBC3_RAM = 0x12,
	ROM_MBC3_RAM_BATTERY = 0x13,
	ROM_MBC5 = 0x19,
	ROM_MBC5_RAM = 0x1A,
	ROM_MBC5_RAM_BATTERY = 0x1B,
	ROM_MBC5_RUMBLE = 0x1C,
	ROM_MBC5_RUMBLE_SRAM = 0x1D,
	ROM_MBC5_RUMBLE_SRAM_BATTERY = 0x1E,
	POCKET_CAMERA = 0x1F,
	BANDAI_TAMA5 = 0xFD,
	HUDSON_HUC3 = 0xFE
};

enum class CartridgeROMSize : uint8
{
	_32KB = 0x00,
	_64KB = 0x01,
	_128KB = 0x02,
	_256KB = 0x03,
	_512KB = 0x04,
	_1MB = 0x05,
	_2MB = 0x06,
	_1_1MB = 0x52,
	_1_2MB = 0x53,
	_1_5MB = 0x54
};

enum class CartridgeRAMSize : uint8
{
	None = 0x00,
	_2KB = 0x01,
	_8KB = 0x02,
	_32KB = 0x03,
	_128KB = 0x04
};

namespace MBCAddresses
{
	static constexpr uint16 CartridgeType = 0x147;
	static constexpr uint16 ROMSize = 0x148;
	static constexpr uint16 RAMSize = 0x149;
}

class Cartridge : public IMemoryElement
{
public:
	~Cartridge();

	void LoadFile(const std::string& filename);
	uint8* GetData() { return m_Data.get(); }

	//Cartrige data
	CartrigeType Type = CartrigeType::ROMOnly;
	CartridgeROMSize RomSize = CartridgeROMSize::_32KB;
	CartridgeRAMSize RamSize = CartridgeRAMSize::None;

	void InitMBC();

	virtual uint8& ReadMemory(uint16 address) override
	{
		return m_MBC->ReadMemory(address);
	}

	virtual void WriteMemory(uint16 address, uint8 value) override
	{
		m_MBC->WriteMemory(address, value);
	}


private:
	std::unique_ptr<uint8[]> m_Data;
	std::unique_ptr<uint8[]> m_RAM;
	std::unique_ptr<IROMMemoryModel> m_MBC;
};