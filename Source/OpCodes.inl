#ifndef OP_CODES_INL
#define OP_CODES_INL
#include "CPU.h"
#include "BinaryOps.h"

using namespace BinaryOps;

inline void GameBoyCPU::NOP()
{
	m_Cycles += 4;
	DEBUGTEXT("NOP");
}

inline void GameBoyCPU::LD_16REG_NN(uint16& Dest, const char* DebugString)
{
	uint16 val = Fetch16BitParameter();
	Dest = val;
	m_Cycles += 4;
	DEBUGTEXT("LD " + std::string(DebugString) + ", NN", val);
}

inline void GameBoyCPU::LD_PTR_8REG(uint16 Address, uint8 Register, const char* DebugString)
{
	WriteMemory(Address, Register);
	m_Cycles += 4;
	DEBUGTEXT("LD " + std::string(DebugString), Address);
}

void GameBoyCPU::LD_PTR_16REG(uint16 Address, uint16 Register, const char* DebugString)
{
	uint8 highPart = Register >> 8;
	uint8 lowPart = Register & 0x00ff;

	WriteMemory(Address, lowPart);
	WriteMemory(Address + 1, highPart);

	m_Cycles += 4;
}

inline void GameBoyCPU::LD_8REG_PTR(uint8& Register, uint16 Address, const char* DebugString)
{
	Register = ReadMemory(Address);
	m_Cycles += 4;
	DEBUGTEXT("LD " + std::string(DebugString), BC);
}

inline void GameBoyCPU::INC_16REG(uint16& Dest, const char* DebugString)
{
	++Dest;
	m_Cycles += 8;
	DEBUGTEXT("INC " + std::string(DebugString));
}

inline void GameBoyCPU::DEC_16REG(uint16& Dest, const char* DebugString)
{
	--Dest;
	m_Cycles += 8;
	DEBUGTEXT("DEC " + std::string(DebugString));
}

inline void GameBoyCPU::INC_8REG(uint8& Dest, const char* DebugString, int AdditionalCycles)
{
	bool bit3Before = GetBit(3, Dest);
	++Dest;
	bool bit3After = GetBit(3, Dest);

	SetValZ(Dest == 0);
	ResetFlagN();
	SetValH(bit3Before != bit3After);
	m_Cycles += 4 + AdditionalCycles;
	DEBUGTEXT("INC " + std::string(DebugString));
}

inline void GameBoyCPU::DEC_8REG(uint8& Dest, const char* DebugString, int AdditionalCycles)
{
	uint8 result = Dest - 1;

	SetValZ(result == 0);
	SetFlagN();
	SetValH(((result ^ 0x01 ^ Dest) & 0x10) == 0x10);
	m_Cycles += 4 + AdditionalCycles;
	Dest = result;

	DEBUGTEXT("DEC " + std::string(DebugString));
}

inline void GameBoyCPU::LD_8REG_N(uint8& Dest, const char* DebugString)
{
	uint8 val = Fetch8BitParameter();
	Dest = val;
	m_Cycles += 4;
	DEBUGTEXT("LD " + std::string(DebugString) + ", N", val);
}

inline void GameBoyCPU::RLA()
{
	bool leftmost = !!(A & 0x80);
	A = A << 1;
	A = SetBit(0, A, GetC());
	ResetFlagZ();
	ResetFlagN();
	ResetFlagH();
	SetValC(leftmost);
	m_Cycles += 4;
	DEBUGTEXT("RLA");
}

inline void GameBoyCPU::RLCA()
{
	bool leftmost = GetBit(7, A);
	A = A << 1;
	A = SetBit(0, A, leftmost);
	ResetFlagZ();
	ResetFlagN();
	ResetFlagH();
	SetValC(leftmost);
	m_Cycles += 4;
	DEBUGTEXT("RLCA");
}

void GameBoyCPU::RLC_8BIT(uint8& Val, int AdditionalCycles)
{
	bool leftmost = !!(Val & 0x80);
	Val = Val << 1;
	Val = SetBit(0, Val, leftmost);
	SetValZ(Val == 0);
	ResetFlagN();
	ResetFlagH();
	SetValC(leftmost);
	m_Cycles += 8 + AdditionalCycles;
	DEBUGTEXT("RLC");
}

void GameBoyCPU::RRCA()
{
	bool rightmost = GetBit(0, A);
	A = A >> 1;
	A = SetBit(7, A, rightmost);
	ResetFlagZ();
	ResetFlagN();
	ResetFlagH();
	SetValC(rightmost);
	m_Cycles += 4;
	DEBUGTEXT("RRCA");
}

void GameBoyCPU::RRC_8BIT(uint8& Val, int AdditionalCycles)
{
	bool rightmost = GetBit(0, Val);
	Val = Val >> 1;
	Val = SetBit(7, Val, rightmost);
	SetValZ(Val == 0);
	ResetFlagN();
	ResetFlagH();
	SetValC(rightmost);
	m_Cycles += 8 + AdditionalCycles;
	DEBUGTEXT("RRC");
}

void GameBoyCPU::SLA_8BIT(uint8& OutVal, int AdditionalCycles)
{
	bool leftmost = GetBit(7, OutVal);
	OutVal = OutVal << 1;
	SetValZ(OutVal == 0);
	SetValC(leftmost);
	ResetFlagH();
	ResetFlagN();
	m_Cycles += 8 + AdditionalCycles;
}

void GameBoyCPU::SRA_8BIT(uint8& OutVal, int AdditionalCycles)
{
	bool rightmost = GetBit(0, OutVal);
	bool leftmost = GetBit(7, OutVal);
	OutVal = OutVal >> 1;
	OutVal = SetBit(7, OutVal, leftmost);
	SetValZ(OutVal == 0);
	SetValC(rightmost);
	ResetFlagH();
	ResetFlagN();
	m_Cycles += 8 + AdditionalCycles;
}

void GameBoyCPU::RRA()
{
	bool rightmost = GetBit(0, A);
	A = A >> 1;
	A = SetBit(7, A, GetC());
	ResetFlagZ();
	ResetFlagN();
	ResetFlagH();
	SetValC(rightmost);
	m_Cycles += 4;
	DEBUGTEXT("RRA");
}

inline void GameBoyCPU::JR()
{
	int8 adder = int8(Fetch8BitParameter());
	PC += adder;
	m_Cycles += 8;
	DEBUGTEXT("JR, N", adder);
}

inline void GameBoyCPU::JR_NZ_NN()
{
	int8 adder = int8(Fetch8BitParameter());
	if (!GetZ())
	{
		PC += adder;
		m_Cycles += 8;
	}
	else
	{
		m_Cycles += 4;
	}
	DEBUGTEXT("JR NZ, NNNN", adder);
}

inline void GameBoyCPU::JR_Z_NN()
{
	int8 adder = int8(Fetch8BitParameter());
	if (GetZ())
	{
		PC += adder;
		m_Cycles += 8;
	}
	else
	{
		m_Cycles += 4;
	}
	DEBUGTEXT("JR Z, NN", adder);
}

inline void GameBoyCPU::LD_HLP_A()
{
	WriteMemory(HL, A);
	HL++;
	m_Cycles += 4;
	DEBUGTEXT("LD (HL+), A");
}

inline void GameBoyCPU::LD_HLM_A()
{
	WriteMemory(HL, A);
	HL--;
	m_Cycles += 4;
	DEBUGTEXT("LD (HL-), A");
}

inline void GameBoyCPU::LD_A_HLP()
{
	A = ReadMemory(HL);
	HL++;
	m_Cycles += 4;
	DEBUGTEXT("LD A, (HL+)");
}

inline void GameBoyCPU::LD_A_HLM()
{
	A = ReadMemory(HL);
	HL--;
	m_Cycles += 4;
	DEBUGTEXT("LD A, (HL-)");
}

void GameBoyCPU::LD_HL_SP_N()
{
	int8 adder = int8(Fetch8BitParameter());
	uint16 result = SP + adder;
	uint16 check = SP ^ adder ^ ((SP + adder) & 0xFFFF);
	((check & 0x100) == 0x100) ? SetFlagC() : ResetFlagC();
	((check & 0x10) == 0x10) ? SetFlagH() : ResetFlagH();

	ResetFlagZ();
	ResetFlagN();

	HL = result;
	m_Cycles += 8;
}

inline void GameBoyCPU::JR_Flag_NN(uint8 Flag, bool FlagSet, const char* DebugString)
{
	int8 adder = int8(Fetch8BitParameter());
	bool Condition = FlagSet ? GetFlag(Flag) : !GetFlag(Flag);
	if (Condition)
	{
		PC += adder;
		m_Cycles += 8;
	}
	else
	{
		m_Cycles += 4;
	}

	DEBUGTEXT("JR " + std::string(DebugString) + ",NN", adder);
}

void GameBoyCPU::JP_Flag_NN(uint8 Flag, bool FlagSet)
{
	bool Condition = FlagSet ? GetFlag(Flag) : !GetFlag(Flag);
	uint16 address = Fetch16BitParameter();
	if (Condition)
	{
		PC = address;
		m_Cycles += 8;
	}
	else
	{
		m_Cycles += 4;
	}
}

inline void GameBoyCPU::LD_8BIT_8BIT(uint8& Dest, uint8 Source, const char* DebugString)
{
	Dest = Source;
	m_Cycles += 4;
	DEBUGTEXT("LD " + std::string(DebugString));
}

inline void GameBoyCPU::LD_16BIT_16BIT(uint16& Dest, uint16 Source, const char* DebugString)
{
	Dest = Source;
	m_Cycles = m_Cycles + 8;
	DEBUGTEXT("PUSH " + std::string(DebugString));
}

inline void GameBoyCPU::LD_FF00_A(uint8 Adder, const char* DebugString)
{
	uint16 address = 0xFF00 + Adder;
	WriteMemory(address, A);
	m_Cycles += 4;
	DEBUGTEXT("LD ($FF00+N), A", Adder);
}

inline void GameBoyCPU::LD_A_FF00(uint8 Adder, const char* DebugString)
{
	uint16 address = 0xFF00 + Adder;
	A = ReadMemory(address);
	m_Cycles += 4;
	DEBUGTEXT("LD A, ($FF00+N)", Adder);
}

inline void GameBoyCPU::ADD_8BIT_8BIT(uint8& Dest, uint8 Add, const char* DebugString)
{
	SetValH(CheckHalfCarry(Dest, Add));
	SetValC((Dest + Add) > 255);
	Dest = Dest + Add;
	m_Cycles += 4;
	SetValZ(Dest == 0);
	ResetFlagN();
	DEBUGTEXT("ADD " + std::string(DebugString));
}

inline void GameBoyCPU::ADD_16BIT_16BIT(uint16& Dest, uint16 Add, const char* DebugString)
{
	uint16 Result = Dest + Add;
	(Result < Dest) ? SetFlagC() : ResetFlagC();
	((Result ^ Dest ^ Add) & 0x1000) ? SetFlagH() : ResetFlagH();

	m_Cycles += 8;
	ResetFlagN();
	Dest = Result;

	DEBUGTEXT("ADD " + std::string(DebugString));
}

inline void GameBoyCPU::ADD_8BIT_N(uint8& Dest, const char* DebugString)
{
	uint8 val = Fetch8BitParameter();
	SetValH(CheckHalfCarry(Dest, val));
	SetValC((Dest + val) > 255);
	Dest = Dest + val;
	m_Cycles += 4;
	SetValZ(Dest == 0);
	ResetFlagN();
	DEBUGTEXT("ADD " + std::string(DebugString) + ", val", val);
}

void GameBoyCPU::ADD_16BIT_N(uint16& Dest, int8 Add, const char* DebugString)
{
	uint16 result = Dest + Add;
	ResetFlagZ();
	ResetFlagN();

	((result & 0xF) < (Dest & 0xF)) ? SetFlagH() : ResetFlagH();
	((result & 0xFF) < (Dest & 0xFF)) ? SetFlagC() : ResetFlagC();

	Dest = result;
	m_Cycles += 12;
}

void GameBoyCPU::ADC_A_8BIT(uint8 Adder)
{
	uint8 carryVal = GetC() ? 0x01 : 0x00;

	SetValH(((int)(A & 0x0F) + (int)(Adder & 0x0F) + (int)carryVal) > 0x0F);
	SetValC(((int)(A & 0xFF) + (int)(Adder & 0xFF) + (int)carryVal) > 0xFF);

	A = A + Adder + carryVal;
	m_Cycles += 4;
	SetValZ(A == 0);
	ResetFlagN();
}

inline void GameBoyCPU::SUB_8BIT(uint8 Reg, const char* DebugString)
{
	SetValH(((A & 0x0F) < (Reg & 0x0F)));
	SetValC(((A & 0xFF) < (Reg & 0xFF)));
	A = A - Reg;
	SetValZ(A == 0);
	SetValN(true);

	m_Cycles += 4;
	DEBUGTEXT("SUB " + std::string(DebugString));
}

void GameBoyCPU::SBC_8BIT(uint8 Reg, const char* DebugString)
{
	int carryVal = GetC() ? 0x01 : 0x00;

	int tempA = A & 0xFF;
	int intAdd = Reg & 0xFF;
	int origA = tempA;

	tempA -= intAdd;
	tempA -= carryVal;
	SetFlagN();
	SetValC(tempA < 0);
	tempA &= 0xFF;
	SetValZ(tempA == 0);
	SetValH(((origA ^ intAdd ^ tempA) & 0x10) == 0x10);

	A = uint8(tempA);

	m_Cycles += 4;
}

inline void GameBoyCPU::AND_8BIT(uint8 Reg, const char* DebugString)
{
	A = A & Reg;
	m_Cycles += 4;
	SetValZ(A == 0);
	ResetFlagN();
	SetFlagH();
	ResetFlagC();
	DEBUGTEXT("AND " + std::string(DebugString));
}

inline void GameBoyCPU::XOR_8BIT(uint8 Reg, const char* DebugString)
{
	A = A ^ Reg;
	SetValZ(A == 0);
	ResetFlags(EFlagMask::FN | EFlagMask::FH | EFlagMask::FC);
	m_Cycles = m_Cycles + 4;
	DEBUGTEXT("XOR " + std::string(DebugString));
}

inline void GameBoyCPU::OR_8BIT(uint8 Reg, const char* DebugString)
{
	A = A | Reg;
	SetValZ(A == 0);
	ResetFlags(EFlagMask::FN | EFlagMask::FH | EFlagMask::FC);
	m_Cycles += 4;
	DEBUGTEXT("OR " + std::string(DebugString));
}

inline void GameBoyCPU::CP(uint8 Reg, const char* DebugString)
{
	uint8 result = A - Reg;
	SetValZ(result == 0);
	SetValN(true);

	SetValC((A & 0xFF) < (Reg & 0xFF));
	SetValH((A & 0x0F) < (Reg & 0x0F));

	m_Cycles += 4;
	DEBUGTEXT("CP " + std::string(DebugString));
}

void GameBoyCPU::SCF()
{
	SetValC(true);
	ResetFlagN();
	ResetFlagH();
	m_Cycles += 4;
}

/*
This requires to adjust register A for decimal operations

*/

void GameBoyCPU::DAA()
{
	int aVal = A;

	if (!GetN())
	{
		if (GetH() || (aVal & 0xF) > 9)
		{
			aVal += 0x06;
		}

		if (GetC() || (aVal > 0x9F))
		{
			aVal += 0x60;
		}
	}
	else
	{
		if (GetH())
		{
			aVal = (aVal - 0x06) & 0xFF;
		}

		if (GetC())
		{
			aVal -= 0x60;
		}
	}

	ResetFlagH();

	if ((aVal & 0x100) == 0x100)
	{
		SetFlagC();
	}

	aVal &= 0xFF;
	SetValZ(aVal == 0x00);
	
	A = (uint8)aVal;
	m_Cycles += 4;

	return;
}

void GameBoyCPU::CCF()
{
	SetValC(!GetC());
	ResetFlagH();
	ResetFlagN();
	m_Cycles += 4;
}

inline void GameBoyCPU::POP(uint16& Reg, const char* DebugString)
{
	Reg = Pull();
	if (&Reg == &AF)
	{
		//it's AF - low 4 bits are always 0
		Reg = Reg & 0xFFF0;
	}

	m_Cycles += 12;
	DEBUGTEXT("POP " + std::string(DebugString));
}

inline void GameBoyCPU::JP()
{
	uint16 address = Fetch16BitParameter();
	PC = address;
	m_Cycles += 8;
	DEBUGTEXT("JP NN", address);
}

void GameBoyCPU::JP_16BIT(uint16 Address)
{
	PC = Address;
	m_Cycles += 4;
}

void GameBoyCPU::PUSH(uint16 Reg, const char* DebugString)
{
	Push(Reg);
	m_Cycles += 16;
	DEBUGTEXT("PUSH " + std::string(DebugString));
}

void GameBoyCPU::RET()
{
	uint16 address = Pull();
	PC = address;
	m_Cycles += 16;
	DEBUGTEXT("RET", address);
}

void GameBoyCPU::RETI()
{
	uint16 address = Pull();
	PC = address;
	m_InterruptEnabled = true;
	m_Cycles += 16;
}

void GameBoyCPU::RET_FLAG(uint8 Flag, bool FlagSet)
{
	bool Condition = FlagSet ? GetFlag(Flag) : !GetFlag(Flag);
	if (Condition)
	{
		uint16 address = Pull();
		PC = address;
		DEBUGTEXT("RET FLAG");
		m_Cycles += 20;
	}
	else
	{
		m_Cycles += 8;
	}
}

void GameBoyCPU::CALL()
{
	uint16 address = Fetch16BitParameter();
	Push(PC);
	PC = address;
	m_Cycles += 16;
	DEBUGTEXT("CALL NN", address);
}

void GameBoyCPU::CALL_FLAG(uint8 Flag, bool FlagSet)
{
	uint16 address = Fetch16BitParameter();

	bool Condition = FlagSet ? GetFlag(Flag) : !GetFlag(Flag);
	if (Condition)
	{
		Push(PC);
		m_Cycles += 16;
		PC = address;
	}
	else
	{
		m_Cycles += 4;
	}
}

void GameBoyCPU::RST(uint8 Val)
{
	uint16 address = Val;
	Push(PC);
	PC = address;
	m_Cycles += 16;
	DEBUGTEXT("RST");
}

void GameBoyCPU::DI()
{
	m_InterruptEnabled = false;
	m_Cycles += 4;
	DEBUGTEXT("DI");
}

void GameBoyCPU::EI()
{
	m_InterruptEnabled = true;
	m_Cycles += 4;
	DEBUGTEXT("EI");
}

void GameBoyCPU::CPL()
{
	A = ~A;
	m_Cycles += 4;
	SetFlagN();
	SetFlagH();
	DEBUGTEXT("CPL");
}

void GameBoyCPU::RL_8BIT(uint8& OutVal, int AdditionalCycles)
{
	bool leftmost = false;

	leftmost = !!(OutVal & 0x80);
	OutVal = OutVal << 1;
	OutVal = SetBit(0, OutVal, GetC());
	SetValZ(OutVal == 0);
	m_Cycles += 8 + AdditionalCycles;

	ResetFlagN();
	ResetFlagH();
	SetValC(leftmost);
}

void GameBoyCPU::RR_8BIT(uint8& OutVal, int AdditionalCycles)
{
	bool rightmost = false;

	rightmost = GetBit(0, OutVal);
	OutVal = OutVal >> 1;
	OutVal = SetBit(7, OutVal, GetC());
	SetValZ(OutVal == 0);
	m_Cycles += 8 + AdditionalCycles;

	ResetFlagN();
	ResetFlagH();
	SetValC(rightmost);
}

void GameBoyCPU::SRL_8BIT(uint8& OutVal, int AdditionalCycles)
{
	bool rightmost = GetBit(0, OutVal);
	SetValC(rightmost);
	OutVal = OutVal >> 1;
	OutVal = SetBit(7, OutVal, false);
	SetValZ(OutVal == 0);
	SetValN(false);
	SetValH(false);
	m_Cycles += 8 + AdditionalCycles;
}

void GameBoyCPU::BIT_8BIT(uint8 bit, uint8 Val)
{
	uint8 mask = 1 << bit;
	bool IsSet = !!(Val & mask);
	SetValZ(!IsSet);
	ResetFlagN();
	SetFlagH();
	m_Cycles += 8;
}

void GameBoyCPU::SWAP_8BIT(uint8& OutVal, int AdditionalCycles)
{
	uint8 bottom = OutVal & 0x0f;
	uint8 top = OutVal & 0xf0;
	top = top >> 4;
	bottom = bottom << 4;
	OutVal = bottom | top;
	m_Cycles += 8 + AdditionalCycles;
	SetValZ(OutVal == 0);

	ResetFlags(EFlagMask::FN | EFlagMask::FH | EFlagMask::FC);
}

void GameBoyCPU::RES_8BIT(uint8 bit, uint8& Val, int AdditionalCycles)
{
	uint8 mask = ~(1 << bit);
	Val = Val & mask;
	m_Cycles += 8 + AdditionalCycles;
}

void GameBoyCPU::SET_8BIT(uint8 bit, uint8& Val, int AdditionalCycles)
{
	uint8 mask = 1 << bit;
	Val = Val | mask;
	m_Cycles += 8 + AdditionalCycles;
}

void GameBoyCPU::HALT()
{
	m_IsHalted = true;
	m_Cycles += 4;
}

void GameBoyCPU::STOP()
{
	HALT();
}

#endif