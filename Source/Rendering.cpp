#include "Rendering.h"
#include "CPU.h"
#include "SDL.h"
#include "Log.h"
#include "Timer.h"
#include "BinaryOps.h"

using namespace BinaryOps;

GBRendering::~GBRendering()
{
	SDL_DestroyWindow(m_Window);
	SDL_DestroyRenderer(m_Renderer);
	SDL_DestroyTexture(m_Texture);
	SDL_Quit();
}

bool GBRendering::Init()
{
	//				A, B, G, R
	m_Colors[0] = { 255, 15,188,155 };
	m_Colors[1] = { 255, 15,172,139 };
	m_Colors[2] = { 255, 48,98,48 };
	m_Colors[3] = { 255, 15, 56, 15 };

	m_Window = SDL_CreateWindow("Gameboy Emulator", 100, 100, ScreenData::SizeX * 4, ScreenData::SizeY * 4, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (m_Window == nullptr)
	{
		SDL_Quit();
		return 1;
	}

	m_Renderer = SDL_CreateRenderer(m_Window, -1, SDL_RENDERER_ACCELERATED /*| SDL_RENDERER_PRESENTVSYNC*/);
	if (m_Renderer == nullptr)
	{
		SDL_DestroyWindow(m_Window);
		SDL_Quit();
		return 1;
	}

	m_Texture = SDL_CreateTexture(m_Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, ScreenData::SizeX, ScreenData::SizeY);
	if (m_Texture == nullptr) {
		SDL_DestroyRenderer(m_Renderer);
		SDL_DestroyWindow(m_Window);
		SDL_Quit();
		return 1;
	}

	return true;
}

void GBRendering::DrawSpriteLine(class GameBoyCPU* CPU, class GPU* InGPU, int32 lineNumber)
{
	const uint32 SpriteSizeBytes = 16;

	uint8 BGPalette = CPU->ReadMemory(MemRegisters::BGPalette, true);
	uint8 BGPaletteArray[4];
	BGPaletteArray[0] = BGPalette & 0x03;
	BGPaletteArray[1] = (BGPalette >> 2) & 0x03;
	BGPaletteArray[2] = (BGPalette >> 4) & 0x03;
	BGPaletteArray[3] = (BGPalette >> 6) & 0x03;

	//loop through sprites
	for (int32 i = 156; i>=0 ; i-=4)
	{
		uint8 YPos = InGPU->m_OAM[i]; //pos - 16
		uint8 XPos = InGPU->m_OAM[i + 1]; // pos - 8

		uint8 SpriteSizeY = GetBit(2, InGPU->m_LCDControl) ? 16 : 8;

		int32 RealY = YPos - 16;
		if ((RealY <= lineNumber) && ((RealY + SpriteSizeY) > lineNumber))
		{
			//am I in the scanline?
			uint8 SpriteIndex = InGPU->m_OAM[i + 2];
			uint8 SpriteFlags = InGPU->m_OAM[i + 3];

			if (SpriteSizeY == 16)
			{
				//LB is 0
				SpriteIndex = SetBit(0, SpriteIndex, false);
			}

			int32 RealX = XPos - 8;
			bool SpritePriority = GetBit(7, SpriteFlags);
			bool YFlip = GetBit(6, SpriteFlags);
			bool XFlip = GetBit(5, SpriteFlags);
			uint8 ObjPalette = GetBit(4, SpriteFlags) ? InGPU->m_OBJPalette1 : InGPU->m_ObjPalette0;

			uint8 PaletteArray[4];
			PaletteArray[0] = 0x00; // transparent
			PaletteArray[1] = (ObjPalette >> 2) & 0x03;
			PaletteArray[2] = (ObjPalette >> 4) & 0x03;
			PaletteArray[3] = (ObjPalette >> 6) & 0x03;

			//direct access to VRAM
			const uint16 TileData = 0x0000;
			uint8 TileYOffset = YFlip ? ((SpriteSizeY - 1) - (lineNumber - RealY)) : (lineNumber - RealY);
			uint16 TilePointer = TileData + (SpriteIndex * SpriteSizeBytes) + (TileYOffset * 2);

			uint8 PixelColorIndexA = InGPU->m_VRAM[TilePointer];
			uint8 PixelColorIndexB = InGPU->m_VRAM[TilePointer + 1];

			//go through the 8 pixels of the sprite
			for (int32 X = 0; X < 8; ++X)
			{
				int32 FlippedX = XFlip ? 7 - X : X;
				int32 ActualCoordX = X + RealX;

				uint8 Col = ((uint8)GetBit(7 - FlippedX, PixelColorIndexA) ) | ((uint8)GetBit(7 - FlippedX, PixelColorIndexB) << 1);
				if ((Col != 0x00) && (ActualCoordX >=0 && ActualCoordX < ScreenData::SizeX))
				{
					//check Priority
					if (!SpritePriority || (SpritePriority && (m_BGLinePixels[ActualCoordX] == 0)))
					{
						m_LineBuffer[ActualCoordX] = m_Colors[PaletteArray[Col]];
					}
				}
			}
		}
	}
}

void GBRendering::DrawLineBackground(class GameBoyCPU* CPU, class GPU* InGPU, int32 lineNumber)
{
	if (InGPU->IsLCDEnabled())
	{
		bool ShouldDraw = false;
		ShouldDraw = InGPU->IsBGEnabled();

		if (!ShouldDraw)
		{
			return;
		}

		//Background
		uint16 MapAddress = 0;
		MapAddress = InGPU->GetBGTileMapAddress();

		uint16 BgWinTileDataAddress = InGPU->GetBGWinTileDataAddress();

		uint8 ScrollY = InGPU->m_ScrollY;
		uint8 ScrollX = InGPU->m_ScrollX;

		uint8 TileScrollY = ScrollY / 8;
		uint8 TileScrollX = ScrollX / 8;

		uint8 Palette = CPU->ReadMemory(MemRegisters::BGPalette, true);

		GBColor PaletteArray[4];
		PaletteArray[0] = m_Colors[Palette & 0x03];
		PaletteArray[1] = m_Colors[(Palette >> 2) & 0x03];
		PaletteArray[2] = m_Colors[(Palette >> 4) & 0x03];
		PaletteArray[3] = m_Colors[(Palette >> 6) & 0x03];


		//Add scroll support
		for (int32 X = 0; X < ScreenData::SizeX; ++X)
		{
			int32 ActualCoordX = ScrollX + X;
			ActualCoordX = ActualCoordX % ScreenData::FullSizeX;
			int32 ActualCoordY = ScrollY + lineNumber;
			ActualCoordY = ActualCoordY % ScreenData::FullSizeY;

			int32 TileX = ActualCoordX / 8;
			int32 TileY = ActualCoordY / 8;

			int32 PixelInTileX = ActualCoordX % 8;
			int32 PixelInTileY = ActualCoordY % 8;

			uint16 BGMapAddress = MapAddress + (TileY * ScreenData::FullTileSizeX) + TileX;
			//direct VRAM access
			uint8 TileIndex = InGPU->m_VRAM[BGMapAddress - 0x8000];

			uint16 TileDataBaseAddress = 0;
			if (GetBit(4, InGPU->m_LCDControl))
			{
				TileDataBaseAddress = BgWinTileDataAddress + (16 * TileIndex) + (2 * PixelInTileY);
			}
			else
			{
				TileDataBaseAddress = BgWinTileDataAddress + (16 * int8(TileIndex)) + (2 * PixelInTileY);
			}

			uint16 TileDataAddressA = TileDataBaseAddress + 0;
			uint16 TileDataAddressB = TileDataBaseAddress + 1;

			uint8 PixelColorIndexA = InGPU->m_VRAM[TileDataAddressA - 0x8000];
			uint8 PixelColorIndexB = InGPU->m_VRAM[TileDataAddressB - 0x8000];

			uint8 Col = ((uint8)GetBit(7 - PixelInTileX, PixelColorIndexA) ) | ((uint8)GetBit(7 - PixelInTileX, PixelColorIndexB) << 1);

			//int32 TileInitAddress = (ScreenData::SizeX * lineNumber) + X;

			m_LineBuffer[X] = PaletteArray[Col];
			m_BGLinePixels[X] = Col;
		}
	}
}

void GBRendering::DrawLineWindow(class GameBoyCPU* CPU, class GPU* InGPU, int32 lineNumber)
{
	if (InGPU->IsLCDEnabled())
	{
		bool ShouldDraw = false;
		ShouldDraw = InGPU->IsWinEnabled();

		if (!ShouldDraw)
		{
			return;
		}

		//Background
		uint16 MapAddress = 0;
		MapAddress = InGPU->GetWinTileMapAddress();

		uint16 BgWinTileDataAddress = InGPU->GetBGWinTileDataAddress();

		uint8 ScrollX = InGPU->m_WinPosX - 7;
		uint8 ScrollY = InGPU->m_WinPosY;

		int WinY = lineNumber - ScrollY;

		if (WinY < 0)
		{
			//not visible
			return;
		}

		uint8 Palette = CPU->ReadMemory(MemRegisters::BGPalette, true);
		uint8 PaletteArray[4];
		PaletteArray[0] = Palette & 0x03;
		PaletteArray[1] = (Palette >> 2) & 0x03;
		PaletteArray[2] = (Palette >> 4) & 0x03;
		PaletteArray[3] = (Palette >> 6) & 0x03;

		//Add scroll support
		for (int32 X = 0; X < ScreenData::SizeX; ++X)
		{
			if (X < ScrollX)
			{
				continue;
			}

			int32 TileX = uint8((X - ScrollX) / 8);
			int32 TileY = uint8(WinY / 8);

			int32 PixelInTileX = (X - ScrollX) % 8;
			int32 PixelInTileY = WinY % 8;

			uint16 WinMapAddress = MapAddress + (TileY * ScreenData::FullTileSizeX) + TileX;
			uint8 TileIndex = CPU->ReadMemory(WinMapAddress, true);

			uint16 TileDataBaseAddress = 0;
			if (GetBit(4, InGPU->m_LCDControl))
			{
				TileDataBaseAddress = BgWinTileDataAddress + (16 * TileIndex) + (2 * PixelInTileY);
			}
			else
			{
				TileDataBaseAddress = BgWinTileDataAddress + (16 * int8(TileIndex)) + (2 * PixelInTileY);
			}

			uint16 TileDataAddressA = TileDataBaseAddress + 0;
			uint16 TileDataAddressB = TileDataBaseAddress + 1;

			uint8 PixelColorIndexA = CPU->ReadMemory(TileDataAddressA, true);
			uint8 PixelColorIndexB = CPU->ReadMemory(TileDataAddressB, true);

			uint8 Col = ((uint8)GetBit(7 - PixelInTileX, PixelColorIndexA)) | ((uint8)GetBit(7 - PixelInTileX, PixelColorIndexB) << 1);
			m_LineBuffer[X] = m_Colors[PaletteArray[Col]];
		}
	}
}

void GBRendering::CopyLineInTexture(int32 lineNumber)
{
	SDL_Rect lineRect;
	lineRect.x = 0;
	lineRect.y = lineNumber;
	lineRect.w = ScreenData::SizeX;
	lineRect.h = 1;
	SDL_UpdateTexture(m_Texture, &lineRect, m_LineBuffer, ScreenData::SizeX * 4);
}

bool GBRendering::PollEvents()
{
	bool shouldGoOn = true;
	//window management
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
		{
			shouldGoOn = false;
		}
	}
	return shouldGoOn;
}

void GBRendering::Render(class GameBoyCPU* CPU)
{
	SDL_RenderCopy(m_Renderer, m_Texture, NULL, NULL);
	SDL_RenderPresent(m_Renderer);
}