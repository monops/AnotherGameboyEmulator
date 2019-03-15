#include "CPU.h"
#include "Log.h"
#include <assert.h>

void GameBoyCPU::CheckRegisters()
{
	if (m_BootSequence)
	{
		if (GetBit(0, m_Memory.Read(MemRegisters::BootLock)) )
		{
			m_BootSequence = false;
		}
	}

}