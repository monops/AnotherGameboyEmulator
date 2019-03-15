#include "GPU.h"
#include "CPU.h"
#include <assert.h>
#include "BinaryOps.h"

using namespace BinaryOps;

GPU::GPU(GameBoyCPU* InCPU) :
	m_CPU(InCPU)
	, m_GPUModeCycles(Timings::VBlankCycles)
{
	m_Rendering.Init();
}

void GPU::RenderScanline()
{
	if (m_LY <= ScreenData::SizeY)
	{
		m_Rendering.InitLine();
		m_Rendering.DrawLineBackground(m_CPU, this, m_LY);
		m_Rendering.DrawLineWindow(m_CPU, this, m_LY);

		if (GetBit(1, m_LCDControl))
		{
			m_Rendering.DrawSpriteLine(m_CPU, this, m_LY);
		}
		
		m_Rendering.CopyLineInTexture(m_LY);
	}
}

bool GPU::IsLCDEnabled()
{
	return GetBit(7, m_CPU->m_Memory.Read(MemRegisters::LCDC));
}

void GPU::FireDMATransfer(uint8 address)
{
	m_DMATransferRemainingCycles = 752;

	uint16 source = (static_cast<uint16>(address) * 0x0100);
	for (uint8 offset = 0x00; offset <= 0x9F; offset++)
	{
		m_OAM[offset] = m_CPU->ReadMemory(source | offset, true);
	}
}

uint8& GPU::ReadMemory(uint16 address)
{
	if (address >= 0x8000 && address <= 0x9FFF)
	{
		return m_VRAM[address - 0x8000];
	}
	else if (address >= 0xFE00 && address <= 0xFE9F)
	{
		return m_OAM[address - 0xFE00];
	}

	static uint8 Zero = 0;
	switch (address)
	{
	case MemRegisters::LCDC:
		return m_LCDControl;
	case MemRegisters::LCDStatus:
		return m_LCDStatus;
	case MemRegisters::ScrollX:
		return m_ScrollX;
	case MemRegisters::ScrollY:
		return m_ScrollY;
	case MemRegisters::WinPosX:
		return m_WinPosX;
	case MemRegisters::WinPosY:
		return m_WinPosY;
	case MemRegisters::BGPalette:
		return m_BGPalette;
	case MemRegisters::ObjPalette0:
		return m_ObjPalette0;
	case MemRegisters::ObjPalette1:
		return m_OBJPalette1;
	case  MemRegisters::LY:
		return m_LY;
	case MemRegisters::LYCompare:
		return m_LYCompare;
	case MemRegisters::DMATransfer:
		return Zero;
	}

	return Zero;
}

void GPU::WriteMemory(uint16 address, uint8 Value)
{
	if (address >= 0x8000 && address <= 0x9FFF)
	{
		m_VRAM[address - 0x8000] = Value;
	}
	else if (address >= 0xFE00 && address <= 0xFE9F)
	{
		m_OAM[address - 0xFE00] = Value;
	}

	switch (address)
	{
	case MemRegisters::LCDC:
	{
		m_LCDControl = Value;
	}
	break;
	case MemRegisters::LCDStatus:
	{
		//no bits 0-2
		m_LCDStatus = Value;// (Value & 0xF8) | (LCDStatus & 0x07);
	}
	break;
	case MemRegisters::ScrollX:
		m_ScrollX = Value;
		break;
	case MemRegisters::ScrollY:
		m_ScrollY = Value;
		break;
	case MemRegisters::WinPosX:
		m_WinPosX = Value;
		break;
	case MemRegisters::WinPosY:
		m_WinPosY = Value;
		break;
	case MemRegisters::BGPalette:
		m_BGPalette = Value;
		break;
	case MemRegisters::ObjPalette0:
		m_ObjPalette0 = Value;
		break;
	case MemRegisters::ObjPalette1:
		m_OBJPalette1 = Value;
		break;
	case MemRegisters::LY:
		m_LY = Value;
		break;
	case MemRegisters::LYCompare:
		m_LYCompare = Value;
		break;
	case MemRegisters::DMATransfer:
		FireDMATransfer(Value);
		break;
	default:
		break;
	}
}

uint16 GPU::GetBGTileMapAddress()
{
	bool BGPosition = GetBit(3, m_LCDControl);

	return BGPosition ? MemAreas::BgTileMapBit31 : MemAreas::BgTileMapBit30;
}

uint16 GPU::GetWinTileMapAddress()
{
	bool WinPosition = GetBit(6, m_LCDControl);
	return WinPosition ? MemAreas::WindowTileMapBit61 : MemAreas::WindowTileMapBit60;
}

bool GPU::IsBGEnabled()
{
	return GetBit(0, m_LCDControl);
}

bool GPU::IsWinEnabled()
{
	return GetBit(0, m_LCDControl) && GetBit(5, m_LCDControl);
}

uint16 GPU::GetBGWinTileDataAddress()
{
	bool BGPosition = GetBit(4, m_LCDControl);

	return BGPosition ? MemAreas::BgWinTileMapUnsignedBit41 : MemAreas::BgWinTileMapSignedBit40;
}

void GPU::Update(uint32 cycles)
{
	if (m_DMATransferRemainingCycles > 0)
	{
		//is this needed?
		m_DMATransferRemainingCycles -= cycles;
	}

	//LCD Managing
	if (IsLCDEnabled())
	{
		//Handle states
		uint8 LCDState = m_LCDStatus;
		uint8 Mode = LCDState & 0x03;

		m_GPUModeCycles += cycles;

		switch (Mode)
		{
		case GPUStates::HBlank:
			//end of horizontal scanline
			if (m_GPUModeCycles >= Timings::HBlankCycles)
			{
				m_GPUModeCycles -= Timings::HBlankCycles;
				m_LY++;
				if (m_LY == 144)
				{
					m_LCDStatus = ((LCDState & ~0x03) | GPUStates::VBlank);
					RenderScreen();
					m_CPU->FireInterrupt(InterruptCodes::VBlank);

					if (GetBit(4, LCDState)) //VBlank Interrupt
					{
						m_CPU->FireInterrupt(InterruptCodes::STAT);
					}
				}
				else
				{
					//Next line
					m_LCDStatus = ((LCDState & ~0x03) | GPUStates::ReadingOAM);
					if (GetBit(5, LCDState)) //OAM Interrupt
					{
						m_CPU->FireInterrupt(InterruptCodes::STAT);
					}
				}
			}
			break;

		case GPUStates::VBlank:
			if (m_GPUModeCycles >= Timings::VBlankCycles)
			{
				m_GPUModeCycles -= Timings::VBlankCycles;

				//10 lines VBlank
				m_LY++;
				if (m_LY == 154)
				{
					//back to top left
					m_LCDStatus = ((LCDState & ~0x03) | GPUStates::ReadingOAM);
					m_LY = 0;
					if (GetBit(5, LCDState)) //OAM Interrupt
					{
						m_CPU->FireInterrupt(InterruptCodes::STAT);
					}
				}
			}
			break;

		case GPUStates::ReadingOAM:
			if (m_GPUModeCycles >= Timings::ReadingOAMCycles)
			{
				m_GPUModeCycles -= Timings::ReadingOAMCycles;
				m_LCDStatus = ((LCDState & ~0x03) | GPUStates::ReadingOAMVRAM);

			}
			break;

		case GPUStates::ReadingOAMVRAM:
			if (m_GPUModeCycles >= Timings::ReadingOAMVRAMCycles)
			{
				m_GPUModeCycles -= Timings::ReadingOAMVRAMCycles;
				RenderScanline();

				m_LCDStatus = ((LCDState & ~0x03) | GPUStates::HBlank);
				if (GetBit(3, LCDState)) //HBlank
				{
					m_CPU->FireInterrupt(InterruptCodes::STAT);
				}
			}
			break;

		default:
			assert(0);
			break;
		}

		// Bit 2 - Coincidence Flag  (0:LYC<>LY, 1:LYC=LY) (Read Only)
		if (m_LYCompare == m_LY)
		{
			m_LCDStatus = SetBit(2, m_LCDStatus, true);
			if (GetBit(6, m_LCDStatus))
			{
				m_CPU->FireInterrupt(InterruptCodes::STAT);
			}
		}
		else
		{
			m_LCDStatus = SetBit(2, m_LCDStatus, false);
		}

		/*
		//LY
		if (GPUModeCycles < 0)
		{
			Memory[MemRegisters::LY]++;

			if (Memory[MemRegisters::LY] == 0x90)
			{
				RenderScreen();
			}

			Memory[MemRegisters::LY] = Memory[MemRegisters::LY] % 154;
			GPUModeCycles = Timings::VBlankCycles;
		}*/
	}
}