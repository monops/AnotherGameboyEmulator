#include "Cartridge.h"
#include <sys/stat.h>

Cartridge::~Cartridge()
{

}


void Cartridge::LoadFile(const std::string& filename)
{
	struct _stat stat_buf;
	int rc = _stat(filename.c_str(), &stat_buf);
	if (rc == 0)
	{
		FILE *f = nullptr;
		if (fopen_s(&f, filename.c_str(), TEXT("rb")) == 0)
		{
			m_Data = std::make_unique<uint8[]>(stat_buf.st_size);

			fread_s(m_Data.get(), stat_buf.st_size, stat_buf.st_size, 1, f);
			fclose(f);

			InitMBC();
		}
	}
}

void Cartridge::InitMBC()
{
	Type = CartrigeType(m_Data[MBCAddresses::CartridgeType]);
	RomSize = CartridgeROMSize(m_Data[MBCAddresses::ROMSize]);
	RamSize = CartridgeRAMSize(m_Data[MBCAddresses::RAMSize]);

	//allocate RAM
	switch (RamSize)
	{
	case CartridgeRAMSize::_2KB:
		m_RAM = std::make_unique<uint8[]>(2 * 1024);
		break;
	case CartridgeRAMSize::_8KB:
		m_RAM = std::make_unique<uint8[]>(8 * 1024);
		break;
	case CartridgeRAMSize::_32KB:
		m_RAM = std::make_unique<uint8[]>(32 * 1024);
		break;
	case CartridgeRAMSize::_128KB:
		m_RAM = std::make_unique<uint8[]>(128 * 1024);
		break;
	default:
		m_RAM.reset();
		break;
	}

	switch (Type)
	{
	case CartrigeType::ROMOnly:
		m_MBC = std::make_unique<MEM_ROMOnly>(m_Data.get(), m_RAM.get());
		break;
	case CartrigeType::ROM_MBC1:
	case CartrigeType::ROM_MBC1_RAM:
	case CartrigeType::ROM_MBC1_RAM_BATTERY:
		m_MBC = std::make_unique<MEM_MBC1>(m_Data.get(), m_RAM.get());
		break;

	case CartrigeType::ROM_MBC2:
	case CartrigeType::ROM_MBC2_BATTERY:
		m_RAM = std::make_unique<uint8[]>(0x200);
		m_MBC = std::make_unique<MEM_MBC2>(m_Data.get(), m_RAM.get());
		break;

	case CartrigeType::ROM_MBC3_RAM:
	case CartrigeType::ROM_MBC3_RAM_BATTERY:
		m_MBC = std::make_unique<MEM_MBC3>(m_Data.get(), m_RAM.get());
		break;
	default:
		break;
	}
}