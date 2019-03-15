#pragma once

#include "Types.h"

namespace EFlagMask
{
	static constexpr uint8 FZ = 1 << 7;
	static constexpr uint8 FN = 1 << 6;
	static constexpr uint8 FH = 1 << 5;
	static constexpr uint8 FC = 1 << 4;
};

namespace MemRegisters
{
	static constexpr uint16 InputRegister = 0xFF00;
	static constexpr uint16 DivRegister = 0xFF04;
	static constexpr uint16 TIMA = 0xFF05;
	static constexpr uint16 TimeModulo = 0xFF06;
	static constexpr uint16 TimeControl = 0xFF07;
	static constexpr uint16 BootLock = 0xFF50;
	static constexpr uint16 LCDC = 0xFF40;
	static constexpr uint16 LCDStatus = 0xFF41;
	static constexpr uint16 LY = 0xFF44;
	static constexpr uint16 LYCompare = 0xFF45;
	static constexpr uint16 DMATransfer = 0xFF46;
	static constexpr uint16 ScrollY = 0xFF42;
	static constexpr uint16 ScrollX = 0xFF43;
	static constexpr uint16 BGPalette = 0xFF47;
	static constexpr uint16 ObjPalette0 = 0xFF48;
	static constexpr uint16 ObjPalette1 = 0xFF49;
	static constexpr uint16 WinPosY = 0xFF4A;
	static constexpr uint16 WinPosX = 0xFF4B;

	static constexpr uint16 InterruptFlags = 0xFF0F;
	static constexpr uint16 InterruptEnabled = 0xFFFF;

	//Sound
	//Pulse A
	static constexpr uint16 NR10_CH1Sweep = 0xFF10;
	static constexpr uint16 NR11_CH1SoundLength = 0xFF11;
	static constexpr uint16 NR12_CH1Envelope = 0xFF12;
	static constexpr uint16 NR13_CH1FrequencyLo = 0xFF13;
	static constexpr uint16 NR14_CH1FrequencyHi = 0xFF14;

	//Pulse B
	static constexpr uint16 NR21_CH2SoundLength = 0xFF16;
	static constexpr uint16 NR22_CH2Envelope = 0xFF17;
	static constexpr uint16 NR23_CH2FrequencyLo = 0xFF18;
	static constexpr uint16 NR24_CH2FrequencyHi = 0xFF19;

	//Wave
	static constexpr uint16 NR30_CH3OnOff = 0xFF1A;
	static constexpr uint16 NR31_CH3SoundLength = 0xFF1B;
	static constexpr uint16 NR32_CH3OutputLevel = 0xFF1C;
	static constexpr uint16 NR33_CH3FrequencyLo = 0xFF1D;
	static constexpr uint16 NR34_CH3FrequencyHi = 0xFF1E;

	//Noise
	static constexpr uint16 NR41_CH4SoundLength = 0xFF20;
	static constexpr uint16 NR42_CH4Envelope = 0xFF21;
	static constexpr uint16 NR43_CH4PolyCounter = 0xFF22;
	static constexpr uint16 NR44_CH4CounterConsecutive = 0xFF23;

	static constexpr uint16 NR50_CHControl_OnOff_Volume = 0xFF24;
	static constexpr uint16 NR51_SoundOutputTerminal = 0xFF25;
	static constexpr uint16 NR52_SoundOnOff = 0xFF26;

	static constexpr uint16 WavePatternBegin = 0xFF30;
	static constexpr uint16 WavePatternEnd = 0xFF3F;
}

namespace MemAreas
{
	static constexpr uint32 MemorySize = 0x10000;
	static constexpr uint16 BgWinTileMapUnsignedBit41 = 0x8000;
	static constexpr uint16 BgWinTileMapSignedBit40 = 0x9000;//0x8800; //documentation is wrong. when this is the case, base is 0x9000 and minimum is -128 at 0x8800
	static constexpr uint16 WindowTileMapBit60 = 0x9800;
	static constexpr uint16 WindowTileMapBit61 = 0x9C00;
	static constexpr uint16 BgTileMapBit30 = 0x9800;
	static constexpr uint16 BgTileMapBit31 = 0x9C00;
}

namespace Timings
{
	static constexpr uint32 GBClockSpeed = 4194304;
	static constexpr float GBClockTime = 1.0f / float(GBClockSpeed);
	static constexpr uint32 VBlankCycles = 456;
	static constexpr uint32 HBlankCycles = 204;
	static constexpr uint32 ReadingOAMCycles = 80;
	static constexpr uint32 ReadingOAMVRAMCycles = 172;
	static constexpr uint32 FrameCycles = 70224;
	static constexpr uint32 DMATransferCycles = 752;

	//timer clock ticks
	static constexpr uint32 Frequency4096 = GBClockSpeed / 4096;
	static constexpr uint32 Frequency16384 = GBClockSpeed / 16384;
	static constexpr uint32 Frequency65536 = GBClockSpeed / 65536;
	static constexpr uint32 Frequency262144 = GBClockSpeed / 262144;
}

namespace ScreenData
{
	static constexpr uint8 SizeX = 160;
	static constexpr uint8 SizeY = 144;
	static constexpr uint32 FullSizeX = 256;
	static constexpr uint32 FullSizeY = 256;
	static constexpr uint8 TileSizeX = SizeX / 8; // 20;
	static constexpr uint8 TileSizeY = SizeY / 8; // 18;
	static constexpr uint8 FullTileSizeX = FullSizeX / 8; // 32;
	static constexpr uint8 FullTileSizeY = FullSizeY / 8; // 32;
}

namespace GPUStates
{
	static constexpr uint8 HBlank = 0x0;
	static constexpr uint8 VBlank = 0x1;
	static constexpr uint8 ReadingOAM = 0x2;
	static constexpr uint8 ReadingOAMVRAM = 0x3;
}

namespace InterruptCodes
{
	static constexpr uint8 VBlank = 0x40;
	static constexpr uint8 STAT = 0x48;
	static constexpr uint8 Timer = 0x50;
	static constexpr uint8 Serial = 0x58;
	static constexpr uint8 Joypad = 0x60;
}
