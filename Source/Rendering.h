#pragma once

#include "Types.h"
#include "Constants.h"

struct GBColor
{
	uint8 A = 255;
	uint8 B;
	uint8 G;
	uint8 R;
};

class GBRendering
{
public:

	~GBRendering();

	bool Init();
	void InitLine()
	{
		memset(m_LineBuffer, 0, ScreenData::SizeX * sizeof(GBColor));
	}
	void Render(class GameBoyCPU* CPU);
	void DrawLineBackground(class GameBoyCPU* CPU, class GPU* InGPU, int32 lineNumber);
	void DrawLineWindow(class GameBoyCPU* CPU, class GPU* InGPU, int32 lineNumber);
	void DrawSpriteLine(class GameBoyCPU* CPU, class GPU* InGPU, int32 lineNumber);
	void CopyLineInTexture(int32 lineNumber);

	bool PollEvents();

private:
	GBColor m_Colors[4];
	GBColor m_LineBuffer[ScreenData::SizeX];
	uint8 m_BGLinePixels[ScreenData::SizeX];

	struct SDL_Window* m_Window = nullptr;
	struct SDL_Renderer* m_Renderer = nullptr;
	struct SDL_Texture* m_Texture = nullptr;
	struct SDL_Texture* m_RenderTarget = nullptr;
};