#pragma once
#include "Types.h"
#include "MemoryElement.h"

class GameBoyCPU;

class GBCounter
{
public:
	GBCounter(int32 InCycles, GameBoyCPU* InCPU);

	bool Tick(uint32 TickCycles);
	void Start() { m_IsRunning = true; }
	void Stop() { m_IsRunning = false; }
	void SetFrequency(int32 InCycles)
	{
		if (m_CounterCycles != InCycles)
		{
			m_CounterCycles = InCycles;
			m_CurrentCycles = InCycles;
		}
	}
	uint8& GetValue() { return m_Value; }
	void SetValue(uint8 val) { m_Value = val; }

private:
	uint8 m_Value;

	int32 m_CounterCycles;
	int32 m_CurrentCycles = 0;
	int32 m_Frequency;
	GameBoyCPU* m_CPU = nullptr;
	bool m_IsRunning;
};

class GBTimer : public IMemoryElement
{
public:
	GBTimer(GameBoyCPU* InCPU);

	void Update(uint32 TickCycles);
	GBCounter& GetCounter() { return m_TimerRegister; }

	virtual uint8& ReadMemory(uint16 address) override;
	virtual void WriteMemory(uint16 address, uint8 Value) override;

private:
	GameBoyCPU* m_CPU = nullptr;
	GBCounter m_DividerRegister;
	GBCounter m_TimerRegister;

	uint8 m_TimerModulo = 0;
	uint8 m_TimerControl = 0;
};