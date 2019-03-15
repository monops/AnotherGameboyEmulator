#include "CPU.h"
#include "Cartridge.h"
#include "Log.h"
#include <assert.h>
#include "Timer.h"
#include "SDL.h"

std::string GameBoyCPU::FlagsToString()
{
	std::string toReturn;
#if DEBUG
	toReturn = std::string("Z:") + (GetZ() ? "1" : "0") + " N:" + (GetN() ? "1" : "0") + " H:" + (GetH() ? "1" : "0") + " C:" + (GetN() ? "1" : "0");
	return toReturn;
#endif
}

std::string GameBoyCPU::RegistersToString()
{
#if DEBUG
	char buffer[10 * 1024];
	sprintf_s(buffer, 10 * 1024, "A:0x%02x F:0x%02x B:0x%02x C:0x%02x D:0x%02x E:0x%02x H:0x%02x L:0x%02x SP:0x%04x", A, F, B, C, D, E, H, L, SP);
	return std::string(buffer);
#else
	return std::string();
#endif
}

void GameBoyCPU::DebugExecution(const std::string& Instruction)
{
#if DEBUG
	if (!m_EnableDebug)
	{
		return;
	}
	Log::log("PC: 0x%04x -> %s -\t Flags: %s -\t Registers %s", PC - 1, Instruction.c_str(), FlagsToString().c_str(), RegistersToString().c_str());
#endif
}

void GameBoyCPU::DebugExecution(const std::string& Instruction, uint8 EightBitParam)
{
#if DEBUG
	if (!m_EnableDebug)
	{
		return;
	}
	Log::log("PC: 0x%04x -> %s : 0x%02x -\tFlags: %s -\t Registers %s", PC - 2, Instruction.c_str(), EightBitParam, FlagsToString().c_str(), RegistersToString().c_str());
#endif
}

void GameBoyCPU::DebugExecution(const std::string& Instruction, int8 EightBitParam)
{
#if DEBUG
	if (!m_EnableDebug)
	{
		return;
	}
	Log::log("PC: 0x%04x -> %s : 0x%02x -\t Flags: %s -\t Registers %s", PC - 2, Instruction.c_str(), EightBitParam, FlagsToString().c_str(), RegistersToString().c_str());
#endif
}

void GameBoyCPU::DebugExecution(const std::string& Instruction, uint16 SixteenBitParam)
{
#if DEBUG
	if (!m_EnableDebug)
	{
		return;
	}
	Log::log("PC: 0x%04x -> %s : 0x%04x -\t Flags: %s -\t Registers %s", PC - 3, Instruction.c_str(), SixteenBitParam, FlagsToString().c_str(), RegistersToString().c_str());
#endif
}

GameBoyCPU::GameBoyCPU()
{

}

GameBoyCPU::~GameBoyCPU()
{

}

void GameBoyCPU::SetCartridge(Cartridge* cart)
{
	m_FitCartridge = cart;
}

void GameBoyCPU::TurnOn()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		return;
	}

	m_GameboyTimer = std::make_unique<GBTimer>(this);
	m_GBGPU = std::make_unique<GPU>(this);
	m_GameboyInput = std::make_unique<GBInput>(this);
	m_GameboySound = std::make_unique<GBSound>(this);

	//setup memory
	m_Memory.RegisterElementRange(0x0000, 0x7FFF, m_FitCartridge);
	m_Memory.RegisterElementRange(0x8000, 0x9FFF, m_GBGPU.get());
	m_Memory.RegisterElementRange(0xA000, 0xBFFF, m_FitCartridge);
	m_Memory.RegisterElement(0xFF00, m_GameboyInput.get());
	m_Memory.RegisterElementRange(0xFE00, 0xFE9F, m_GBGPU.get());

	m_Memory.RegisterElementRange(0xFF40, 0xFF4C, m_GBGPU.get());
	m_Memory.RegisterElementRange(0xFF4E, 0xFF4F, m_GBGPU.get());
	m_Memory.RegisterElementRange(0xFF04, 0xFF07, m_GameboyTimer.get());

	m_Memory.RegisterElementRange(0xFF51, 0xFF55, m_GBGPU.get());
	m_Memory.RegisterElementRange(0xFF57, 0xFF6B, m_GBGPU.get());
	m_Memory.RegisterElementRange(0xFF6D, 0xFF6F, m_GBGPU.get());

	//Sound
	m_Memory.RegisterElementRange(0xFF10, 0xFF14, m_GameboySound.get());
	m_Memory.RegisterElementRange(0xFF16, 0xFF1E, m_GameboySound.get());
	m_Memory.RegisterElementRange(0xFF20, 0xFF26, m_GameboySound.get());
	m_Memory.RegisterElementRange(0xFF30, 0xFF3F, m_GameboySound.get());

	m_Memory.Write(MemRegisters::BootLock, 0);
	m_Memory.Write(MemRegisters::LCDC, 0);
	m_Memory.Write(MemRegisters::LCDStatus, 0x85);
	m_Memory.Write(MemRegisters::ScrollX, 0);
	m_Memory.Write(MemRegisters::ScrollY, 0);
	m_Memory.Write(MemRegisters::LY, 153);
	m_Memory.Write(MemRegisters::TIMA, 0);
	m_Memory.Write(MemRegisters::TimeModulo, 0);

	m_BootSequence = true;
	PC = 0;
}

uint8& GameBoyCPU::ReadMemory(uint16 address, bool skipCycles)
{
	if (!skipCycles)
	{
		m_Cycles += 4;
	}
	
	if (m_BootSequence)
	{
		if (Between<uint16>(0x0000, 0x00ff, address))
		{
			return s_Firmware[address];
		}
		else
		{
			return m_Memory.Read(address);
		}
	}
	else
	{
		return m_Memory.Read(address);
	}
}

void GameBoyCPU::WriteMemory(uint16 address, uint8 value, bool skipCycles)
{
	if (!skipCycles)
	{
		m_Cycles += 4;
	}

	m_Memory.Write(address, value);
}

void GameBoyCPU::FireInterrupt(uint8 InterruptCode)
{
	static constexpr uint8 VBlank = 0x40;
	static constexpr uint8 STAT = 0x48;
	static constexpr uint8 Timer = 0x50;
	static constexpr uint8 Serial = 0x58;
	static constexpr uint8 Joypad = 0x60;

	uint8 interruptState = ReadMemory(MemRegisters::InterruptFlags, true);
	if (InterruptCode == InterruptCodes::VBlank) { interruptState = SetBit(0, interruptState, true); }
	else if (InterruptCode == InterruptCodes::STAT) { interruptState = SetBit(1, interruptState, true); }
	else if (InterruptCode == InterruptCodes::Timer) { interruptState = SetBit(2, interruptState, true); }
	else if (InterruptCode == InterruptCodes::Serial) { interruptState = SetBit(3, interruptState, true); }
	else if (InterruptCode == InterruptCodes::Joypad) { interruptState = SetBit(4, interruptState, true); }

	m_IsHalted = false;

	WriteMemory(MemRegisters::InterruptFlags, interruptState, true);
}

void GameBoyCPU::ManageInterrupts()
{
	if (!m_InterruptEnabled)
	{
		return;
	}

	uint8 IF = ReadMemory(MemRegisters::InterruptFlags, true);
	uint8 IE = ReadMemory(MemRegisters::InterruptEnabled, true);

	uint8 activeInterrupts = ((IE & IF) & 0x0F);
	if (activeInterrupts > 0x00)
	{
		m_InterruptEnabled = false;
		Push(PC);

		if (GetBit(0, activeInterrupts))
		{
			//VBlank
			PC = InterruptCodes::VBlank;
			IF = SetBit(0, IF, false);
		}
		else if (GetBit(1, activeInterrupts))
		{
			//LCD Status
			PC = InterruptCodes::STAT;
			IF = SetBit(1, IF, false);
		}
		else if (GetBit(2, activeInterrupts))
		{
			//Timer
			PC = InterruptCodes::Timer;
			IF = SetBit(2, IF, false);
		}
		else if (GetBit(3, activeInterrupts))
		{
			//Serial
			PC = InterruptCodes::Serial;
			IF = SetBit(3, IF, false);
		}
		else if (GetBit(4, activeInterrupts))
		{
			//Joypad
			PC = InterruptCodes::Joypad;
			IF = SetBit(4, IF, false);
		}

		WriteMemory(MemRegisters::InterruptFlags, IF, true);
	}
}

void GameBoyCPU::RenderScreen()
{
	m_GBGPU->RenderScreen();
}

void GameBoyCPU::RenderScanline()
{
	m_GBGPU->RenderScanline();
}

void GameBoyCPU::Run(bool SkipBootstrap)
{
	if (SkipBootstrap)
	{
		PC = 0x100;
		A = 0x01;
		F = 0xB0;
		BC = 0x0013;
		DE = 0x00D8;
		HL = 0x014D;
		SP = 0xFFFE;
		m_BootSequence = false;

		m_Memory.Write(0xFF05, 0x00); // TIMA
		m_Memory.Write(0xFF06, 0x00); // TMA
		m_Memory.Write(0xFF07, 0x00); // TAC
		m_Memory.Write(0xFF10, 0x80); // NR10
		m_Memory.Write(0xFF11, 0xBF); // NR11
		m_Memory.Write(0xFF12, 0xF3); // NR12
		m_Memory.Write(0xFF14, 0xBF); // NR14
		m_Memory.Write(0xFF16, 0x3F); // NR21
		m_Memory.Write(0xFF17, 0x00); // NR22
		m_Memory.Write(0xFF19, 0xBF); // NR24
		m_Memory.Write(0xFF1A, 0x7F); // NR30
		m_Memory.Write(0xFF1B, 0xFF); // NR31
		m_Memory.Write(0xFF1C, 0x9F); // NR32
		m_Memory.Write(0xFF1E, 0xBF); // NR33
		m_Memory.Write(0xFF20, 0xFF); // NR41
		m_Memory.Write(0xFF21, 0x00); // NR42
		m_Memory.Write(0xFF22, 0x00); // NR43
		m_Memory.Write(0xFF23, 0xBF); // NR30
		m_Memory.Write(0xFF24, 0x77); // NR50
		m_Memory.Write(0xFF25, 0xF3); // NR51
		m_Memory.Write(0xFF26, 0xF1); // NR52
		m_Memory.Write(0xFF40, 0x91); // LCDC
		m_Memory.Write(0xFF41, 0x85); // STAT
		m_Memory.Write(0xFF42, 0x00); // SCY
		m_Memory.Write(0xFF43, 0x00); // SCX
		m_Memory.Write(0xFF45, 0x00); // LYC
		m_Memory.Write(0xFF47, 0xFC); // BGP
		m_Memory.Write(0xFF48, 0xFF); // OBP0
		m_Memory.Write(0xFF49, 0xFF); // OBP1
		m_Memory.Write(0xFF4A, 0x00); // WY
		m_Memory.Write(0xFF4B, 0x00); // WX
		m_Memory.Write(0xFFFF, 0x00); // IE
	}

	int32 instrCount = 0;

	bool goOn = true;
	Timer FrameTimer;
	FrameTimer.Start();
	while (goOn)
	{
		m_Cycles = 0;

		ManageInterrupts();
		ExecutePC();

		m_FullCycles += m_Cycles;
		m_FrameCycles += m_Cycles;
		CheckRegisters();
		m_GBGPU->Update(m_Cycles);
		m_GameboyTimer->Update(m_Cycles);
		m_GameboySound->Update(m_Cycles);
		instrCount++;

		if (m_FrameCycles >= Timings::FrameCycles)
		{
			m_GameboyInput->Update();
			goOn = m_GBGPU->PollEvents();
			m_FrameCycles = 0;

			while (true)
			{
				double Time = FrameTimer.End();
				if (Time >= (1.0f / 60.0f))
				{ 
					FrameTimer.Start();
					break;
				}
			}
		}
	}
}

bool GameBoyCPU::CheckHalfCarry(uint8 val1, uint8 val2)
{
	return (((val1 & 0xf) + (val2 & 0xf)) & 0x10) == 0x10;
}

bool GameBoyCPU::CheckHalfCarry(uint16 val1, uint16 val2)
{
	return (((val1 & 0xff) + (val2 & 0xff)) & 0x100) == 0x100;
}

void GameBoyCPU::Push(uint16 value)
{
	//high byte
	uint8 highByte = (value >> 8) & 0x00ff;
	uint8 lowByte = value & 0x00ff;
	SP--;
	m_Memory.Write(SP, highByte);
	SP--;
	m_Memory.Write(SP, lowByte);
}

uint16 GameBoyCPU::Pull()
{
	uint16 val = m_Memory.Read(SP + 1);
	val = val << 8;
	val |= m_Memory.Read(SP);
	SP += 2;

	return val;
}

void GameBoyCPU::ExecutePC()
{
	if (m_IsHalted)
	{
		NOP();
		return;
	}

	uint8 Instruction = FetchInstruction();
	switch (Instruction)
	{

	// 00
	case 0x00: // NOP 
	{ NOP(); } break;
	case 0x01: // LD BC, NN
	{ LD_16REG_NN(BC, "BC"); }	break;
	case 0x02: // LD (BC), A
	{ LD_PTR_8REG(BC, A, "(BC), A"); } break;
	case 0x03: // INC BC
	{ INC_16REG(BC, "BC"); } break;
	case 0x04: // INC B
	{ INC_8REG(B, "B");	} break;
	case 0x05: // DEC B
	{ DEC_8REG(B, "B");	} break;
	case 0x06: // LD B, N
	{ LD_8REG_N(B, "B"); } break;
	case 0x07: //RLCA
	{ RLCA(); } break;
	case 0x08: //LD (NN), SP
	{ LD_PTR_16REG(Fetch16BitParameter(), SP, "SP"); } break;
	case 0x09: // ADD HL, BC
	{ ADD_16BIT_16BIT(HL, BC, "HL, BC"); } break;
	case 0x0A: // LD A, (BC)
	{ LD_8REG_PTR(A, BC, "A, (HL)"); } break;
	case 0x0B: //DEC BC
	{ DEC_16REG(BC, "BC"); } break;
	case 0x0C: // INC C
	{ INC_8REG(C, "C");	} break;
	case 0x0D: // DEC C
	{ DEC_8REG(C, "C");	} break;
	case 0x0E: // LD C, N
	{ LD_8REG_N(C, "C"); } break;
	case 0x0F: // LD C, N
	{ RRCA(); } break;

	//10
	case 0x10: // STOP
	{ STOP(); } break;
	case 0x11: // LD DE, NN
	{ LD_16REG_NN(DE, "DE"); }	break;
	case 0x12: // LD (DE), A
	{ LD_PTR_8REG(DE, A, "(DE), A"); } break;
	case 0x13: // INC DE
	{ INC_16REG(DE, "DE"); } break;
	case 0x14: // INC D
	{ INC_8REG(D, "D");	} break;
	case 0x15: // DEC D
	{ DEC_8REG(D, "D");	} break;
	case 0x19: // ADD HL, DE
	{ ADD_16BIT_16BIT(HL, DE, "HL, DE"); } break;
	case 0x1B: //DEC DE
	{ DEC_16REG(DE, "DE"); } break;
	case 0x1C: // INC E
	{ INC_8REG(E, "E");	} break;
	case 0x1D: // DEC E
	{ DEC_8REG(E, "E");	} break;
	case 0x16: // LD D, N
	{ LD_8REG_N(D, "D"); } break;
	case 0x17: //RLA
	{ RLA(); }	break;
	case 0x18: // JR Address
	{ JR(); } break;
	case 0x1A: // LD A, (DE)
	{ LD_8REG_PTR(A, DE, "A, (DE)"); } break;
	case 0x1E: // LD E, N
	{ LD_8REG_N(E, "E"); } break;
	case 0x1F: // RRA
	{ RRA(); } break;

	//20
	case 0x20: // JR NZ, Address
	{ JR_Flag_NN(EFlagMask::FZ, false, "NZ"); }	break;
	case 0x21: // LD HL, NN
	{ LD_16REG_NN(HL, "HL"); }	break;
	case 0x22: // LD (HL+), A
	{ LD_HLP_A(); }	break;
	case 0x23: // INC HL
	{ INC_16REG(HL, "HL"); } break;
	case 0x24: // INC H
	{ INC_8REG(H, "H");	} break;
	case 0x25: // DEC H
	{ DEC_8REG(H, "H");	} break;
	case 0x26: // LD H, N
	{ LD_8REG_N(H, "H"); } break;
	case 0x27: //DAA
	{ DAA(); } break;
	case 0x28: // JR Z, Address
	{ JR_Flag_NN(EFlagMask::FZ, true, "Z"); } break;
	case 0x29: // ADD HL, HL
	{ ADD_16BIT_16BIT(HL, HL, "HL, HL"); } break;
	case 0x2A : //LD A, (HL+)
	{ LD_A_HLP(); } break;
	case 0x2B: //DEC HL
	{ DEC_16REG(HL, "HL"); } break;
	case 0x2C: // INC L
	{ INC_8REG(L, "L");	} break;
	case 0x2D: // DEC L
	{ DEC_8REG(L, "L");	} break;
	case 0x2E: // LD L, N
	{ LD_8REG_N(L, "H"); } break;
	case 0x2F: //CPL
	{ CPL(); } break;

	//30
	case 0x30: // JR NC, Address
	{ JR_Flag_NN(EFlagMask::FC, false, "Z"); } break;
	case 0x31: // LD SP, NN
	{ LD_16REG_NN(SP, "SP"); }	break;
	case 0x32: // LD (HL-), A
	{ LD_HLM_A(); }	break;
	case 0x33: // INC SP
	{ INC_16REG(SP, "SP"); } break;
	case 0x34: // INC (HL)
	{ INC_8REG(ReadMemory(HL), "Memory[HL]", 4); } break;
	case 0x35: // DEC (HL)
	{ DEC_8REG(ReadMemory(HL), "Memory[HL]", 4); } break;
	case 0x36: // LD (HL), N
	{ LD_PTR_8REG(HL, Fetch8BitParameter(), "(HL), N"); } break;
	case 0x37: // SCF
	{ SCF(); } break;
	case 0x38: // JR C, Address
	{ JR_Flag_NN(EFlagMask::FC, true, "Z"); } break;
	case 0x39: // ADD HL, SP
	{ ADD_16BIT_16BIT(HL, SP, "HL, SP"); } break;
	case 0x3A: //LD A, (HL-)
	{ LD_A_HLM(); } break;
	case 0x3B: //DEC SP
	{ DEC_16REG(SP, "SP"); } break;
	case 0x3C: // INC A
	{ INC_8REG(A, "A");	} break;
	case 0x3D: // DEC A
	{ DEC_8REG(A, "A");	} break;
	case 0x3E: // LD A, N
	{ LD_8REG_N(A, "A"); } break;
	case 0x3F: // CCF
	{ CCF(); } break;

	//40
	case 0x40: // LD B, B
	{ LD_8BIT_8BIT(B, B, "B, B"); }	break;
	case 0x41: // LD B, C
	{ LD_8BIT_8BIT(B, C, "B, C"); }	break;
	case 0x42: // LD B, D
	{ LD_8BIT_8BIT(B, D, "B, D"); }	break;
	case 0x43: // LD B, E
	{ LD_8BIT_8BIT(B, E, "B, E"); }	break;
	case 0x44: // LD B, H
	{ LD_8BIT_8BIT(B, H, "B, H"); }	break;
	case 0x45: // LD B, L
	{ LD_8BIT_8BIT(B, L, "B, L"); }	break;
	case 0x46: // LD B, (HL)
	{ LD_8BIT_8BIT(B, ReadMemory(HL), "B, (HL)"); }	break;
	case 0x47: // LD B, A
	{ LD_8BIT_8BIT(B, A, "B, A"); }	break;
	case 0x48: // LD C, B
	{ LD_8BIT_8BIT(C, B, "C, B"); }	break;
	case 0x49: // LD C, C
	{ LD_8BIT_8BIT(C, C, "C, C"); }	break;
	case 0x4A: // LD C, D
	{ LD_8BIT_8BIT(C, D, "C, D"); }	break;
	case 0x4B: // LD C, E
	{ LD_8BIT_8BIT(C, E, "C, E"); }	break;
	case 0x4C: // LD C, H
	{ LD_8BIT_8BIT(C, H, "C, H"); }	break;
	case 0x4D: // LD C, L
	{ LD_8BIT_8BIT(C, L, "C, L"); }	break;
	case 0x4E: // LD C, (HL)
	{ LD_8BIT_8BIT(C, ReadMemory(HL), "C, (HL)"); }	break;
	case 0x4F: // LD C, A
	{ LD_8BIT_8BIT(C, A, "C, A"); }	break;

	//50
	case 0x50: // LD D, B
	{ LD_8BIT_8BIT(D, B, "D, B"); }	break;
	case 0x51: // LD D, C
	{ LD_8BIT_8BIT(D, C, "D, C"); }	break;
	case 0x52: // LD D, D
	{ LD_8BIT_8BIT(D, D, "D, D"); }	break;
	case 0x53: // LD D, E
	{ LD_8BIT_8BIT(D, E, "D, E"); }	break;
	case 0x54: // LD D, H
	{ LD_8BIT_8BIT(D, H, "D, H"); }	break;
	case 0x55: // LD D, L
	{ LD_8BIT_8BIT(D, L, "D, L"); }	break;
	case 0x56: // LD D, (HL)
	{ LD_8BIT_8BIT(D, ReadMemory(HL), "D, (HL)"); }	break;
	case 0x57: // LD D, A
	{ LD_8BIT_8BIT(D, A, "D, A"); }	break;
	case 0x58: // LD E, B
	{ LD_8BIT_8BIT(E, B, "E, B"); }	break;
	case 0x59: // LD E, C
	{ LD_8BIT_8BIT(E, C, "E, C"); }	break;
	case 0x5A: // LD E, D
	{ LD_8BIT_8BIT(E, D, "E, D"); }	break;
	case 0x5B: // LD E, E
	{ LD_8BIT_8BIT(E, E, "E, E"); }	break;
	case 0x5C: // LD E, H
	{ LD_8BIT_8BIT(E, H, "E, H"); }	break;
	case 0x5D: // LD E, L
	{ LD_8BIT_8BIT(E, L, "E, L"); }	break;
	case 0x5E: // LD E, (HL)
	{ LD_8BIT_8BIT(E, ReadMemory(HL), "E, (HL)"); }	break;
	case 0x5F: // LD E, A

	//60
	{ LD_8BIT_8BIT(E, A, "E, A"); }	break;
	case 0x60: // LD H, B
	{ LD_8BIT_8BIT(H, B, "H, B"); }	break;
	case 0x61: // LD H, C
	{ LD_8BIT_8BIT(H, C, "H, C"); }	break;
	case 0x62: // LD H, D
	{ LD_8BIT_8BIT(H, D, "H, D"); }	break;
	case 0x63: // LD H, E
	{ LD_8BIT_8BIT(H, E, "H, E"); }	break;
	case 0x64: // LD H, H
	{ LD_8BIT_8BIT(H, H, "H, H"); }	break;
	case 0x65: // LD H, L
	{ LD_8BIT_8BIT(H, L, "H, L"); }	break;
	case 0x66: // LD H, (HL)
	{ LD_8BIT_8BIT(H, ReadMemory(HL), "H, (HL)"); }	break;
	case 0x67: // LD H, A
	{ LD_8BIT_8BIT(H, A, "H, A"); }	break;
	case 0x68: // LD L, B
	{ LD_8BIT_8BIT(L, B, "L, B"); }	break;
	case 0x69: // LD L, C
	{ LD_8BIT_8BIT(L, C, "L, C"); }	break;
	case 0x6A: // LD L, D
	{ LD_8BIT_8BIT(L, D, "L, D"); }	break;
	case 0x6B: // LD L, E
	{ LD_8BIT_8BIT(L, E, "L, E"); }	break;
	case 0x6C: // LD L, H
	{ LD_8BIT_8BIT(L, H, "L, H"); }	break;
	case 0x6D: // LD L, L
	{ LD_8BIT_8BIT(L, L, "L, L"); }	break;
	case 0x6E: // LD L, (HL)
	{ LD_8BIT_8BIT(L, ReadMemory(HL), "L, (HL)"); }	break;
	case 0x6F: // LD L, A
	{ LD_8BIT_8BIT(L, A, "L, A"); }	break;

	//70
	case 0x70: // LD (HL), B
	{ LD_PTR_8REG(HL, B, "(HL), B"); } break;
	case 0x71: // LD (HL), C
	{ LD_PTR_8REG(HL, C, "(HL), C"); } break;
	case 0x72: // LD (HL), D
	{ LD_PTR_8REG(HL, D, "(HL), D"); } break;
	case 0x73: // LD (HL), E
	{ LD_PTR_8REG(HL, E, "(HL), E"); } break;
	case 0x74: // LD (HL), H
	{ LD_PTR_8REG(HL, H, "(HL), H"); } break;
	case 0x75: // LD (HL), L
	{ LD_PTR_8REG(HL, L, "(HL), L"); } break;
	case 0x76: //HALT
	{ HALT(); } break;
	case 0x77: // LD (HL), A
	{ LD_PTR_8REG(HL, A, "(HL), A"); } break;
	case 0x78: // LD A, B
	{ LD_8BIT_8BIT(A, B, "A, B"); }	break;
	case 0x79: // LD A, C
	{ LD_8BIT_8BIT(A, C, "A, C"); }	break;
	case 0x7A: // LD A, D
	{ LD_8BIT_8BIT(A, D, "A, D"); }	break;
	case 0x7B: // LD A, E
	{ LD_8BIT_8BIT(A, E, "A, E"); }	break;
	case 0x7C: // LD A, H
	{ LD_8BIT_8BIT(A, H, "A, H"); }	break;
	case 0x7D: // LD A, L
	{ LD_8BIT_8BIT(A, L, "A, L"); }	break;
	case 0x7E: // LD A, (HL)
	{ LD_8REG_PTR(A, HL, "A, (HL)"); } break;
	case 0x7F: // LD A, A
	{
		//A = A;
		m_Cycles += 4;
		DebugExecution("LD A, A");
	}
	break;

	//80
	case 0x80: // ADD A, B
	{ ADD_8BIT_8BIT(A, B, "A, B"); } break;
	case 0x81: // ADD A, C
	{ ADD_8BIT_8BIT(A, C, "A, C"); } break;
	case 0x82: // ADD A, D
	{ ADD_8BIT_8BIT(A, D, "A, D"); } break;
	case 0x83: // ADD A, E
	{ ADD_8BIT_8BIT(A, E, "A, E"); } break;
	case 0x84: // ADD A, H
	{ ADD_8BIT_8BIT(A, H, "A, H"); } break;
	case 0x85: // ADD A, L
	{ ADD_8BIT_8BIT(A, L, "A, L"); } break;
	case 0x86: // ADD A, (HL)
	{ ADD_8BIT_8BIT(A, ReadMemory(HL), "A, (HL)"); } break;
	case 0x87: // ADD A, A
	{ ADD_8BIT_8BIT(A, A, "A, A"); } break;
	case 0x88: // ADC A, B
	{ ADC_A_8BIT(B); } break;
	case 0x89: // ADC A, C
	{ ADC_A_8BIT(C); } break;
	case 0x8A: // ADC A, D
	{ ADC_A_8BIT(D); } break;
	case 0x8B: // ADC A, E
	{ ADC_A_8BIT(E); } break;
	case 0x8C: // ADC A, H
	{ ADC_A_8BIT(H); } break;
	case 0x8D: // ADC A, L
	{ ADC_A_8BIT(L); } break;
	case 0x8E: // ADC A, (HL)
	{ ADC_A_8BIT(ReadMemory(HL)); } break;
	case 0x8F: // ADC A, A
	{ ADC_A_8BIT(A); } break;

	//90
	case 0x90: // SUB B
	{ SUB_8BIT(B, "B"); } break;
	case 0x91: // SUB C
	{ SUB_8BIT(C, "C"); } break;
	case 0x92: // SUB D
	{ SUB_8BIT(D, "D"); } break;
	case 0x93: // SUB E
	{ SUB_8BIT(E, "E"); } break;
	case 0x94: // SUB H
	{ SUB_8BIT(H, "H"); } break;
	case 0x95: // SUB L
	{ SUB_8BIT(L, "L"); } break;
	case 0x96: // SUB (HL)
	{ SUB_8BIT(ReadMemory(HL), "(HL)"); } break;
	case 0x97: // SUB A
	{ SUB_8BIT(A, "A"); } break;
	case 0x98: // SBC B
	{ SBC_8BIT(B, "B"); } break;
	case 0x99: // SBC C
	{ SBC_8BIT(C, "C"); } break;
	case 0x9A: // SBC D
	{ SBC_8BIT(D, "D"); } break;
	case 0x9B: // SBC E
	{ SBC_8BIT(E, "E"); } break;
	case 0x9C: // SBC H
	{ SBC_8BIT(H, "H"); } break;
	case 0x9D: // SBC L
	{ SBC_8BIT(L, "L"); } break;
	case 0x9E: // SBC (HL)
	{ SBC_8BIT(ReadMemory(HL), "(HL)"); } break;
	case 0x9F: // SBC A
	{ SBC_8BIT(A, "A"); } break;

	//A0
	case 0xA0: // AND B
	{ AND_8BIT(B, "B"); } break;
	case 0xA1: // AND C
	{ AND_8BIT(C, "C"); } break;
	case 0xA2: // AND D
	{ AND_8BIT(D, "D"); } break;
	case 0xA3: // AND E
	{ AND_8BIT(E, "E"); } break;
	case 0xA4: // AND H
	{ AND_8BIT(H, "H"); } break;
	case 0xA5: // AND L
	{ AND_8BIT(L, "L"); } break;
	case 0xA6: // AND (HL)
	{ AND_8BIT(ReadMemory(HL), "(HL)"); } break;
	case 0xA7: // AND A
	{ AND_8BIT(A, "A"); } break;
	case 0xA8: // XOR B
	{ XOR_8BIT(B, "B");	} break;
	case 0xA9: // XOR C
	{ XOR_8BIT(C, "C");	} break;
	case 0xAA: // XOR D
	{ XOR_8BIT(D, "D");	} break;
	case 0xAB: // XOR E
	{ XOR_8BIT(E, "E");	} break;
	case 0xAC: // XOR H
	{ XOR_8BIT(H, "H");	} break;
	case 0xAD: // XOR L
	{ XOR_8BIT(L, "L");	} break;
	case 0xAE: // XOR (HL)
	{ XOR_8BIT(ReadMemory(HL), "(HL)");	} break;
	case 0xAF: // XOR A
	{ XOR_8BIT(A, "A");	} break;

	//B0
	case 0xB0: //OR B
	{ OR_8BIT(B, "B"); } break;
	case 0xB1: //OR C
	{ OR_8BIT(C, "C"); } break;
	case 0xB2: //OR D
	{ OR_8BIT(D, "D"); } break;
	case 0xB3: //OR E
	{ OR_8BIT(E, "E"); } break;
	case 0xB4: //OR H
	{ OR_8BIT(H, "H"); } break;
	case 0xB5: //OR L
	{ OR_8BIT(L, "L"); } break;
	case 0xB6: //OR (HL)
	{ OR_8BIT(ReadMemory(HL), "A"); } break;
	case 0xB7: //OR A
	{ OR_8BIT(A, "A"); } break;
	case 0xB8: // CP B
	{ CP(B, "B"); }	break;
	case 0xB9: // CP C
	{ CP(C, "C"); }	break;
	case 0xBA: // CP D
	{ CP(D, "D"); }	break;
	case 0xBB: // CP E
	{ CP(E, "E"); }	break;
	case 0xBC: // CP H
	{ CP(H, "H"); }	break;
	case 0xBD: // CP L
	{ CP(L, "L"); }	break;
	case 0xBE: // CP (HL)
	{ CP(ReadMemory(HL), "(HL)"); }	break;
	case 0xBF: // CP A

	//C0
	{ CP(A, "A"); }	break;
	case 0xC0: // RET NZ
	{ RET_FLAG(EFlagMask::FZ, false); }	break;
	case 0xC1: // POP BC
	{ POP(BC, "BC"); } break;
	case 0xC2: //JP NZ, NN
	{ JP_Flag_NN(EFlagMask::FZ, false); } break;
	case 0xC3: // JP NN
	{ JP(); } break;
	case 0xC4: // CALL NZ, NN
	{ CALL_FLAG(EFlagMask::FZ, false); } break;
	case 0xC5: // PUSH BC
	{ PUSH(BC, "BC"); }	break;
	case 0xC6: // ADD A, N
	{ ADD_8BIT_N(A, "A"); }	break;
	case 0xC7: //RST 00h
	{ RST(0x00); } break;
	case 0xC8: // RET Z
	{ RET_FLAG(EFlagMask::FZ, true); }	break;
	case 0xC9: // RET
	{ RET(); }	break;
	case 0xCA: //JP Z, NN
	{ JP_Flag_NN(EFlagMask::FZ, true); } break;
	case 0xCB: // CB instructions
	{
		uint8 secondPart = Fetch8BitParameter(true);
		ManageCBInstruction(secondPart);
		DEBUGTEXT("CB instruction", secondPart);
	}
	break;
	case 0xCC: // CALL Z, NN
	{ CALL_FLAG(EFlagMask::FZ, true); } break;
	case 0xCD: // CALL NN
	{ CALL(); }	break;
	case 0xCE: // ADC A, N
	{ ADC_A_8BIT(Fetch8BitParameter()); } break;
	case 0xCF: //RST 08h
	{ RST(0x08); } break;

	//D0
	case 0xD0: // RET NC
	{ RET_FLAG(EFlagMask::FC, false); }	break;
	case 0xD1: // POP DE
	{ POP(DE, "DE"); } break;
	case 0xD2: //JP NC, NN
	{ JP_Flag_NN(EFlagMask::FC, false); } break;
	case 0xD4: // CALL NC, NN
	{ CALL_FLAG(EFlagMask::FC, false); } break;
	case 0xD5: // PUSH DE
	{ PUSH(DE, "DE"); }	break;
	case 0xD6: // SUB N
	{ SUB_8BIT(Fetch8BitParameter(), "NN"); } break;
	case 0xD7: //RST 10h
	{ RST(0x10); } break;
	case 0xD8: // RET C
	{ RET_FLAG(EFlagMask::FC, true); }	break;
	case 0xD9: // RETI
	{ RETI(); }	break;
	case 0xDA: //JP C, NN
	{ JP_Flag_NN(EFlagMask::FC, true); } break;
	case 0xDC: // CALL C, NN
	{ CALL_FLAG(EFlagMask::FC, true); } break;
	case 0xDE: // SBC N
	{ SBC_8BIT(Fetch8BitParameter(), "N"); } break;
	case 0xDF: //RST 18h
	{ RST(0x18); } break;

	//E0
	case 0xE0: // LD ($FF00+N),A
	{ LD_FF00_A(Fetch8BitParameter(), ""); } break;
	case 0xE1: // POP HL
	{ POP(HL, "HL"); } break;
	case 0xE2: // LD ($FF00+C),A
	{ LD_FF00_A(C, ""); } break;
	case 0xE5: // PUSH HL
	{ PUSH(HL, "HL"); }	break;
	case 0xE6: // AND N
	{ AND_8BIT(Fetch8BitParameter(), "N"); } break;
	case 0xE7: //RST 20h
	{ RST(0x20); } break;
	case 0xE8: // ADD SP, n
	{ ADD_16BIT_N(SP, int8(Fetch8BitParameter()), "ADD SP, N"); } break;
	case 0xE9: //JP (HL)
	{ JP_16BIT(HL); } break;
	case 0xEA: // LD (NN), A
	{ LD_PTR_8REG(Fetch16BitParameter(), A, "A"); }	break;
	case 0xEC: //not existing
	{ } break;
	case 0xEE: // XOR #
	{ XOR_8BIT(Fetch8BitParameter(), "N");	} break;
	case 0xEF: //RST 28h
	{ RST(0x28); } break;

	//F0
	case 0xF0: // LD A,($FF00+N)
	{ LD_A_FF00(Fetch8BitParameter(), ""); } break;
	case 0xF1: // POP AF
	{ POP(AF, "AF"); } break;
	case 0xF2: // LD A,($FF00+C)
	{ LD_A_FF00(C, ""); } break;
	case 0xF3: // DI
	{ DI(); } break;
	case 0xF5: // PUSH AF
	{ PUSH(AF, "AF"); }	break;
	case 0xF6: //OR N
	{ OR_8BIT(Fetch8BitParameter(), "N"); } break;
	case 0xF7: //RST 30h
	{ RST(0x30); } break;
	case 0xF8: //LD, HL, SP+n
	{ LD_HL_SP_N(); } break;
	case 0xF9: // LD SP, HL
	{ LD_16BIT_16BIT(SP, HL, "SP, HL"); } break;
	case 0xFA: // LD A, NN
	{ LD_8BIT_8BIT(A, ReadMemory(Fetch16BitParameter()), "A, NN"); } break;
	case 0xFB: // EI
	{ EI(); } break;
	case 0xFE: // CP N
	{ CP(Fetch8BitParameter(), "N"); } break;
	case 0xFF: //RST 0x38
	{ RST(0x38); } break;
	default:
		assert(0);
		break;
	}

}