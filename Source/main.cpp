#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "CPU.h"
#include "Cartridge.h"
#include "Timer.h"
#include <commdlg.h>


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	GameBoyCPU CPU;
	Cartridge cart;
	Timer::InitTimer();

	if (IsDebuggerPresent())
	{
		//cart.LoadFile("..\\ROMS\\Metroid2.gb");
		//cart.LoadFile("..\\ROMS\\Gargoyle.gb");
		cart.LoadFile("..\\ROMS\\Castlevania2.gb");
		//cart.LoadFile("..\\ROMS\\TMNT2.gb");
		//cart.LoadFile("..\\ROMS\\TMNT1.gb");
		//cart.LoadFile("..\\ROMS\\DonkeyKong.gb");
		//cart.LoadFile("..\\ROMS\\RType.gb");
		//cart.LoadFile("..\\ROMS\\DuckTales2.gb");
		//cart.LoadFile("..\\ROMS\\PokemonRed.gb");
		//Cart.LoadFile("..\\ROMS\\Probotector.gb");
		//cart.LoadFile("..\\ROMS\\SuperMarioLand.gb");
		//cart.LoadFile("..\\ROMS\\SuperMarioLand2.gb");
		//cart.LoadFile("..\\ROMS\\SuperMarioLand3.gb");
		//cart.LoadFile("..\\ROMS\\Tetris.gb");
		//cart.LoadFile("..\\ROMS\\DrMario.gb");
		//cart.LoadFile("..\\ROMS\\F1Race.gb");

		//cart.LoadFile("..\\ROMS\\Tests\\instr_timing\\instr_timing.gb");
		//cart.LoadFile("..\\ROMS\\Mooneye\\acceptance\\call_cc_timing.gb");
		//cart.LoadFile("..\\ROMS\\Mooneye\\acceptance\\bits\\reg_f.gb");
		//cart.LoadFile("..\\ROMS\\Mooneye\\acceptance\\bits\\mem_oam.gb");
		//cart.LoadFile("..\\ROMS\\Mooneye\\acceptance\\div_timing.gb");
		//cart.LoadFile("..\\ROMS\\Tests\\cpu_instr\\cpu_instrs.gb");
		//cart.LoadFile("..\\ROMS\\Tests\\mem_timing\\mem_timing.gb");

		//Individual tests - ALL PASSED!
		//cart.LoadFile("..\\ROMS\\Tests\\cpu_instr\\individual\\03-op sp,hl.gb");
		//cart.LoadFile("..\\ROMS\\Tests\\cpu_instr\\individual\\04-op r,imm.gb");
		//cart.LoadFile("..\\ROMS\\Tests\\cpu_instr\\individual\\11-op a,(hl).gb");
		//cart.LoadFile("..\\ROMS\\Tests\\mem_timing\\individual\\01-read_timing.gb");

		//Mem Timing
		//cart.LoadFile("..\\ROMS\\Tests\\mem_timing\\individual\\01-read_timing.gb");
	}
	else
	{
		char fileName[256];

		OPENFILENAME openFileStruct;
		ZeroMemory(&openFileStruct, sizeof(openFileStruct));
		openFileStruct.lStructSize = sizeof(openFileStruct);
		openFileStruct.hwndOwner = NULL;
		openFileStruct.lpstrFile = fileName;
		openFileStruct.lpstrFile[0] = '\0';
		openFileStruct.nMaxFile = sizeof(fileName);
		openFileStruct.lpstrFilter = "GB\0*.gb\0";
		openFileStruct.nFilterIndex = 1;
		openFileStruct.lpstrFileTitle = NULL;
		openFileStruct.nMaxFileTitle = 0;
		openFileStruct.lpstrInitialDir = ".\\";
		openFileStruct.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		if (GetOpenFileNameA(&openFileStruct))
		{
			cart.LoadFile(fileName);
		}
		else
		{
			return 0;
		}
	}

	CPU.SetCartridge(&cart);
	
	CPU.TurnOn();
	CPU.Run(true);
	return 0;
}