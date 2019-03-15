#include "GBTimer.h"
#include "CPU.h"
#include "Log.h"
#include "BinaryOps.h"

GBCounter::GBCounter(int32 InCycles, GameBoyCPU* InCPU) :
	m_CounterCycles(InCycles)
	, m_CurrentCycles(InCycles)
	, m_CPU(InCPU)
	, m_IsRunning(true)
	, m_Value(0)
{}

bool GBCounter::Tick(uint32 TickCycles)
{
	if (!m_IsRunning)
	{
		return false;
	}

	m_CurrentCycles -= TickCycles;
	while (m_CurrentCycles <= 0)
	{
		m_CurrentCycles += m_CounterCycles;
		m_Value++;
		if (m_Value == 0)
		{
			return true;
		}
	}
	return false;
}

GBTimer::GBTimer(GameBoyCPU* InCPU) :
	m_DividerRegister(Timings::Frequency16384, InCPU)
	, m_TimerRegister(Timings::Frequency4096, InCPU)
	, m_CPU(InCPU)
{
	m_TimerRegister.Stop();
}

void GBTimer::Update(uint32 TickCycles)
{
	m_DividerRegister.Tick(TickCycles);

	if (m_TimerRegister.Tick(TickCycles))
	{
		//loop completed
		m_TimerRegister.SetValue(m_TimerModulo);
		m_CPU->FireInterrupt(InterruptCodes::Timer);
	}
}

uint8& GBTimer::ReadMemory(uint16 address)
{
	switch (address)
	{
	case MemRegisters::DivRegister:
		return m_DividerRegister.GetValue();
	case MemRegisters::TIMA:
		return m_TimerRegister.GetValue();
	case MemRegisters::TimeModulo:
		return m_TimerModulo;
	case MemRegisters::TimeControl:
		return m_TimerControl;
	default:
		static uint8 Zero = 0;
		return Zero;
	}
}

void GBTimer::WriteMemory(uint16 address, uint8 Value)
{
	switch (address)
	{
	case MemRegisters::DivRegister:
		m_DividerRegister.SetValue(0x00);
		break;
	case MemRegisters::TIMA:
		m_TimerRegister.SetValue(Value);
		break;
	case MemRegisters::TimeModulo:
		m_TimerModulo = Value;
		break;
	case MemRegisters::TimeControl:
	{
		m_TimerControl = Value;
		if (GetBit(2, Value))
		{
			m_TimerRegister.Start();
		}
		else
		{
			m_TimerRegister.Stop();
		}

		uint32 timerFreq = Value & 0x03;
		switch (timerFreq)
		{
		case 0:
			m_TimerRegister.SetFrequency(Timings::Frequency4096);
			break;
		case 1:
			m_TimerRegister.SetFrequency(Timings::Frequency262144);
			break;
		case 2:
			m_TimerRegister.SetFrequency(Timings::Frequency65536);
			break;
		case 3:
			m_TimerRegister.SetFrequency(Timings::Frequency16384);
			break;
		default:
			break;
		}
	}
		break;
	default:
		break;
	}
}