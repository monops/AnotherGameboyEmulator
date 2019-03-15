#pragma once

#include "Types.h"
#include "MemoryElement.h"

class IROMMemoryModel : public IMemoryElement
{
public:
	virtual uint8& ReadMemory(uint16 address) = 0;
	virtual void WriteMemory(uint16 address, uint8 Value) = 0;

	IROMMemoryModel(uint8* InROM, uint8* InRAM):
		m_ROM(InROM)
		, m_RAM(InRAM)
	{}


protected:
	uint8* m_ROM = nullptr;
	uint8* m_RAM = nullptr;
	bool m_IsRAMEnabled = false;
};

class GameBoyMemory : public IMemoryElement
{
public:
	GameBoyMemory()
	{
		RegisterElementRange(0x0000, 0xFFFF, this);
	}

	void RegisterElementRange(uint16 From, uint16 To, IMemoryElement* Pointer);
	void RegisterElement(uint16 Address, IMemoryElement* Pointer);

	uint8& Read(uint16 address);
	void Write(uint16 address, uint8 Value);

private:
	virtual uint8& ReadMemory(uint16 address) override;
	virtual void WriteMemory(uint16 address, uint8 Value) override;

	IMemoryElement* m_MemoryMap[0x10000];

	uint8 m_InternalRAM[0x2000]; //8k Internal RAM -> 0xC000
	uint8 m_HInternalRAM[0x7F]; // High internal RAM -> 0xFF80
	uint8 m_IsBooting;
	uint8 m_InterruptFlags;
	uint8 m_InterruptEnabled;
};


class MEM_ROMOnly : public IROMMemoryModel
{
public:
	MEM_ROMOnly(uint8* InROM, uint8* InRAM) : IROMMemoryModel(InROM, InRAM)
	{

	}

	virtual uint8& ReadMemory(uint16 address) override;
	virtual void WriteMemory(uint16 address, uint8 Value) override;
};

class MEM_MBC2 : public IROMMemoryModel
{
public:
	MEM_MBC2(uint8* pROM, uint8* pRAM);
	~MEM_MBC2() = default;

	virtual uint8& ReadMemory(uint16 address) override;
	virtual void WriteMemory(uint16 address, uint8 Value) override;

private:
	uint8 m_ROMBank;
};

class MEM_MBC1 : public IROMMemoryModel
{
public:
	MEM_MBC1(uint8* pROM, uint8* pRAM);
	~MEM_MBC1() = default;

	virtual uint8& ReadMemory(uint16 address) override;
	virtual void WriteMemory(uint16 address, uint8 Value) override;

private:
	uint8 m_ROMBankLower;
	uint8 m_ROMRAMBankUpper;
	uint8 m_ROMRAMMode;
};

class MEM_MBC3 : public IROMMemoryModel
{
public:
	MEM_MBC3(uint8* pROM, uint8* pRAM);
	~MEM_MBC3() = default;

	virtual uint8& ReadMemory(uint16 address) override;
	virtual void WriteMemory(uint16 address, uint8 Value) override;

private:
	uint8 m_ROMBank;
	uint8 m_RAMBank;
	uint8 m_RTCRegisters[0x05];
};