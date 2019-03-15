#pragma once
#include "Types.h"
#include "Firmware.h"
#include "Rendering.h"
#include "Timer.h"
#include "GBTimer.h"
#include "GBSound.h"
#include "MemoryModel.h"
#include "GPU.h"
#include <vector>
#include <memory.h>
#include "Constants.h"
#include "Input.h"


class GameBoyCPU
{
public:
	friend class GBRendering;
	static constexpr uint32 s_FirmwareSize = 0x100;

	GameBoyCPU();
	~GameBoyCPU();

	void TurnOn();
	void SetCartridge(class Cartridge* cart);
	void Run(bool SkipBootstrap);
private:
	//registers
	//double registers are inverted to accommodate PC byte order
	union
	{
		struct  
		{
			uint8 F;
			uint8 A;
		};

		uint16 AF = 0;
	};

	union
	{
		struct
		{
			uint8 C;
			uint8 B;
		};

		uint16 BC = 0;
	};

	union
	{
		struct
		{
			uint8 E;
			uint8 D;
		};

		uint16 DE = 0;
	};

	union
	{
		struct
		{
			uint8 L;
			uint8 H;
		};

		uint16 HL = 0;
	};

	uint16 PC = 0x100;
	uint16 SP = 0;

	class Cartridge* m_FitCartridge = nullptr;

	void ExecutePC();
	uint8 FetchInstruction()
	{
		uint8 instruction = ReadMemory(PC, true);
		PC++;
		return instruction;
	}

	uint16 Fetch16BitParameter()
	{
		uint16 param;
		uint8 low = ReadMemory(PC);
		uint8 high = ReadMemory(PC + 1);

		param = low + (high << 8);
		PC += 2;
		return param;
	}
	uint8 Fetch8BitParameter(bool skipCycles = false)
	{
		uint8 param = ReadMemory(PC, skipCycles);
		PC++;
		return param;
	}

	void Push(uint16 value);
	uint16 Pull();

	bool CheckHalfCarry(uint8 val1, uint8 val2);
	bool CheckHalfCarry(uint16 val1, uint16 val2);

	void ManageCBInstruction(uint8 secondPart);

	template<typename TYPE>
	bool Between(TYPE A, TYPE B, TYPE val)
	{
		return ((val >= A) && (val <= B));
	}

	void SetFlagZ()
	{
		F |= 0x80;
	}

	void SetFlagN()
	{
		F |= 0x40;
	}

	void SetFlagH()
	{
		F |= 0x20;
	}

	void SetFlagC()
	{
		F |= 0x10;
	}

	void ResetFlagZ()
	{
		F &= ~0x80;
	}

	void ResetFlagN()
	{
		F &= ~0x40;
	}

	void ResetFlagH()
	{
		F &= ~0x20;
	}

	void ResetFlagC()
	{
		F &= ~0x10;
	}

	bool GetZ()
	{
		return F & EFlagMask::FZ;
	}

	bool GetN()
	{
		return F & EFlagMask::FN;
	}

	bool GetH()
	{
		return F & EFlagMask::FH;
	}

	bool GetC()
	{
		return F & EFlagMask::FC;
	}

	void SetFlags(uint8 mask)
	{
		F |= mask;
	}

	void ResetFlags(uint8 mask)
	{
		F &= ~mask;
	}

	bool GetFlag(uint8 mask)
	{
		return !!(F & mask);
	}

	void SetValZ(bool val)
	{
		val ? SetFlagZ() : ResetFlagZ();
	}

	void SetValN(bool val)
	{
		val ? SetFlagN() : ResetFlagN();
	}

	void SetValH(bool val)
	{
		val ? SetFlagH() : ResetFlagH();
	}

	void SetValC(bool val)
	{
		val ? SetFlagC() : ResetFlagC();
	}

private:
	void RenderScreen();
	void RenderScanline();
	void CheckRegisters();
	void ManageInterrupts();

public:
	__forceinline uint8& ReadMemory(uint16 address, bool skipCycles = false);
	__forceinline void WriteMemory(uint16 address, uint8 value, bool skipCycles = false);

	GameBoyMemory m_Memory;
	std::unique_ptr<GPU> m_GBGPU;

	void FireInterrupt(uint8 InterruptCode);

private:
	uint32 m_Cycles = 0;
	bool m_BootSequence = true;
	bool m_InterruptEnabled = false;
	bool m_IsHalted = false;

	//Instructions
	__forceinline void NOP();
	__forceinline void LD_16REG_NN(uint16& Dest, const char* DebugString);
	__forceinline void LD_8REG_N(uint8& Dest, const char* DebugString);
	__forceinline void LD_PTR_8REG(uint16 Address, uint8 Register, const char* DebugString);
	__forceinline void LD_PTR_16REG(uint16 Address, uint16 Register, const char* DebugString);
	__forceinline void LD_8REG_PTR(uint8& Register, uint16 Address, const char* DebugString);
	__forceinline void LD_HLP_A();
	__forceinline void LD_HLM_A();
	__forceinline void LD_A_HLP();
	__forceinline void LD_A_HLM();
	__forceinline void LD_HL_SP_N();

	__forceinline void LD_8BIT_8BIT(uint8& Dest, uint8 Source, const char* DebugString);
	__forceinline void LD_16BIT_16BIT(uint16& Dest, uint16 Source, const char* DebugString);
	__forceinline void LD_FF00_A(uint8 Adder, const char* DebugString);
	__forceinline void LD_A_FF00(uint8 Adder, const char* DebugString);

	__forceinline void INC_16REG(uint16& Dest, const char* DebugString);
	__forceinline void INC_8REG(uint8& Dest, const char* DebugString, int AdditionalCycles = 0);
	__forceinline void DEC_16REG(uint16& Dest, const char* DebugString);
	__forceinline void DEC_8REG(uint8& Dest, const char* DebugString, int AdditionalCycles = 0);
	__forceinline void RLA();
	__forceinline void RRA();
	__forceinline void RLCA();
	__forceinline void RRCA();
	__forceinline void SCF();
	__forceinline void DAA();
	__forceinline void CCF();
	__forceinline void RLC_8BIT(uint8& Val, int AdditionalCycles = 0);
	__forceinline void RRC_8BIT(uint8& Val, int AdditionalCycles = 0);

	__forceinline void JR();
	__forceinline void JR_NZ_NN();
	__forceinline void JR_Z_NN();
	__forceinline void JR_Flag_NN(uint8 Flag, bool FlagSet, const char* DebugString);
	__forceinline void JP();
	__forceinline void JP_16BIT(uint16 Address);
	__forceinline void JP_Flag_NN(uint8 Flag, bool FlagSet);

	__forceinline void ADD_8BIT_8BIT(uint8& Dest, uint8 Add, const char* DebugString);
	__forceinline void ADD_16BIT_16BIT(uint16& Dest, uint16 Add, const char* DebugString);
	__forceinline void ADD_8BIT_N(uint8& Dest, const char* DebugString);
	__forceinline void ADD_16BIT_N(uint16& Dest, int8 Add, const char* DebugString);

	__forceinline void ADC_A_8BIT(uint8 Adder);
	__forceinline void SUB_8BIT(uint8 Reg, const char* DebugString);
	__forceinline void SBC_8BIT(uint8 Reg, const char* DebugString);
	__forceinline void CPL();

	__forceinline void AND_8BIT(uint8 Reg, const char* DebugString);
	__forceinline void XOR_8BIT(uint8 Reg, const char* DebugString);
	__forceinline void OR_8BIT(uint8 Reg, const char* DebugString);

	__forceinline void CP(uint8 Reg, const char* DebugString);

	__forceinline void POP(uint16& Reg, const char* DebugString);
	__forceinline void PUSH(uint16 Reg, const char* DebugString);
	__forceinline void RET();
	__forceinline void RET_FLAG(uint8 Flag, bool FlagSet);
	__forceinline void RETI();

	__forceinline void CALL();
	__forceinline void CALL_FLAG(uint8 Flag, bool FlagSet);
	__forceinline void DI();
	__forceinline void EI();
	__forceinline void RST(uint8 Val);
	__forceinline void HALT();
	__forceinline void STOP();

	//CB instructions
	__forceinline void SRL_8BIT(uint8& OutVal, int AdditionalCycles = 0);
	__forceinline void RL_8BIT(uint8& OutVal, int AdditionalCycles = 0);
	__forceinline void RR_8BIT(uint8& OutVal, int AdditionalCycles = 0);
	__forceinline void BIT_8BIT(uint8 bit, uint8 Val);
	__forceinline void SWAP_8BIT(uint8& OutVal, int AdditionalCycles = 0);
	__forceinline void RES_8BIT(uint8 bit, uint8& Val, int AdditionalCycles = 0);
	__forceinline void SET_8BIT(uint8 bit, uint8& Val, int AdditionalCycles = 0);
	__forceinline void SLA_8BIT(uint8& OutVal, int AdditionalCycles = 0);
	__forceinline void SRA_8BIT(uint8& OutVal, int AdditionalCycles = 0);

	//DEBUG
	uint64 m_FullCycles = 0;
	uint32 m_FrameCycles = 0;
	bool m_EnableDebug = false;
	std::string FlagsToString();
	std::string RegistersToString();
	void DebugExecution(const std::string& Instruction);
	void DebugExecution(const std::string& Instruction, uint8 EightBitParam);
	void DebugExecution(const std::string& Instruction, int8 EightBitParam);
	void DebugExecution(const std::string& Instruction, uint16 SixteenBitParam);

	//Timer
	std::unique_ptr<GBTimer> m_GameboyTimer;
	std::unique_ptr<GBInput> m_GameboyInput;
	std::unique_ptr<GBSound> m_GameboySound;

	//PERFORMANCE
	Timer m_RenderScanTimer;
};

#define ENABLE_DEBUGTEXT 0

#if DEBUG && ENABLE_DEBUGTEXT
#define DEBUGTEXT(...) DebugExecution(__VA_ARGS__);
#else
#define DEBUGTEXT(...)
#endif

#include "OpCodes.inl"