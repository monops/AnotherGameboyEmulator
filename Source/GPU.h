#pragma once

#include "Types.h"
#include "MemoryElement.h"
#include "Rendering.h"

class GPU : public IMemoryElement
{
public:
	friend class GBRendering;

	GPU(class GameBoyCPU* InCPU);

	virtual uint8& ReadMemory(uint16 address) override;
	virtual void WriteMemory(uint16 address, uint8 value) override;

	void Update(uint32 Cycles);
	void RenderScreen()
	{
		m_Rendering.Render(m_CPU);
	}
	void RenderScanline();
	bool IsLCDEnabled();
	bool PollEvents()
	{
		return m_Rendering.PollEvents();
	}

	uint16 GetBGTileMapAddress();
	uint16 GetWinTileMapAddress();
	uint16 GetBGWinTileDataAddress();
	bool IsBGEnabled();
	bool IsWinEnabled();

private:

	void FireDMATransfer(uint8 address);

	GBRendering m_Rendering;
	int32 m_GPUModeCycles;
	int32 m_DMATransferRemainingCycles = 0;

	GameBoyCPU* m_CPU = nullptr;
	uint8 m_VRAM[0x2000];
	uint8 m_OAM[0x100];

	uint8 m_LY;
	uint8 m_LYCompare;
	uint8 m_LCDControl = 0;
	uint8 m_LCDStatus = 0;
	uint8 m_ScrollX;
	uint8 m_ScrollY;
	uint8 m_WinPosX;
	uint8 m_WinPosY;
	uint8 m_BGPalette;
	uint8 m_ObjPalette0;
	uint8 m_OBJPalette1;
};