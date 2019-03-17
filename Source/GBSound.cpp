#include "GBSound.h"
#include "CPU.h"
#include "SDL.h"
#include "BinaryOps.h"
#include <algorithm>

using namespace BinaryOps;

SoundChannel::SoundChannel(GBSound* SoundSystem) :
	m_SoundSystem(SoundSystem)
{

}

bool SoundChannel::IsOn()
{
	return GetBit(7, m_CHFrequencyHiControl);
}

bool Wave::IsOn()
{
	return GetBit(7, m_CHOnOff);
}

uint16 SoundChannel::GetFrequency()
{
	uint16 Freq = m_CHFrequencyLo;
	uint16 FreqHi = m_CHFrequencyHiControl & 0x07;
	Freq = (FreqHi << 8) | Freq;

	return Freq;
}

void SoundChannel::SetFrequency(uint16 Frequency)
{
	m_CHFrequencyLo = Frequency & 0x0F;
	uint16 NewFreqHI = Frequency & 0x0700;

	//setting those three bits only
	m_CHFrequencyHiControl = (m_CHFrequencyHiControl & ~0x0700) | NewFreqHI;
}

uint8 SoundChannel::GetLength()
{
	return 64 - (m_CHSoundLength & 0x3F);
}

void SoundChannel::SetLength(uint8 newLength)
{
	m_CHSoundLength = SetMask(0x3F, m_CHSoundLength, newLength + 64);
}

bool SoundChannel::GetCounterConsecutive()
{
	return GetBit(6, m_CHFrequencyHiControl);
}

uint8 SoundChannel::GetVolumeRegister()
{
	uint8 Volume = (m_CHEnvelope >> 4) & 0x0F;
	return Volume;
}

void SoundChannel::SetVolumeRegister(uint8 volume)
{	
	volume = (volume > 0) ? volume : 0;
	m_CHEnvelope = SetMask(0xf0, m_CHEnvelope, (volume << 4) & 0xF0);
}


float SoundChannel::GetVolume(int32 Cycles)
{
	uint8 Volume = GetVolumeRegister();
	return float(Volume) / 15.0f;
}

float SoundChannel::GetEnvelope(int32 Cycles)
{
	int32 value = GetVolumeSweepCount();
	return float(value) / 7.0f;
}

bool SoundChannel::GetVolumeSweepDirection()
{
	return GetBit(3, m_CHEnvelope);
}

int32 SoundChannel::GetVolumeSweepCount()
{
	int32 SweepCount = m_CHEnvelope & 0x07;
	return SweepCount;
}

void SoundChannel::SetVolumeSweepCount(int32 count)
{
	count = (count < 0) ? 0 : count;
	m_CHEnvelope = SetMask(0x07, m_CHEnvelope, (uint8)count);
}

int32 PulseA::GetFrequencySweepShiftCount()
{
	int32 SweepCount = m_CHSweep & 0x07;
	return SweepCount;
}

bool PulseA::GetFrequenctSweepDirection()
{
	return GetBit(3, m_CHSweep);
}

int32 PulseA::GetFrequencySweepTime()
{
	int32 SweepTime = (m_CHSweep >> 4) & 0x07;
	return SweepTime;
}

float Wave::GetVolume(int32 Cycles)
{
	uint8 Volume = (m_CHEnvelope >> 5) & 0x03;

	switch(Volume)
	{
	case 0:
		return 0.0f;
	case 1:
		return 1.0f;
	case 2:
		return 0.5f;
	case 3:
		return 0.25f;
	}

	return 1.0f;
}

uint8 PulseGeneric::GetPulseRatio()
{
	uint8 RatioVal = (m_CHSoundLength & 0xC0) >> 6;
	return RatioVal;
}

GBSound::GBSound( GameBoyCPU* InCPU):
	CPU(InCPU)
	, m_PulseA(this)
	, m_PulseB(this)
	, m_Wave(this)
	, m_Noise(this)
{
	SDL_AudioSpec Want, Have;
	SDL_zero(Want);
	Want.freq = Frequency;
	Want.format = AUDIO_F32;
	Want.channels = 2;
	Want.samples = BufferSize;
	Want.callback = nullptr;
	Want.userdata = this;

	m_Device = SDL_OpenAudioDevice(NULL, 0, &Want, &Have, 0);
	SDL_PauseAudio(0);
	SDL_PauseAudioDevice(m_Device, 0);
}

GBSound::~GBSound()
{
	SDL_CloseAudioDevice(m_Device);
}

void GBSound::AudioCallback(void*  userdata,
	Uint8* stream,
	int    len)
{
	GBSound* Device = (GBSound*)userdata;

	uint32 sampleCount = len / sizeof(SoundSample);
	SoundSample* sampleStream = (SoundSample*)stream;

	for (uint32 i = 0; i < sampleCount; ++i)
	{
		if (i < Device->m_CurrentSample)
		{
			sampleStream[i] = Device->m_GeneratedSamples[i];
		}
		else
		{
			sampleStream[i].m_Left = 0.0f;
			sampleStream[i].m_Right = 0.0f;
		}
	}
	Device->m_CurrentSample = 0;
}

uint8& GBSound::ReadMemory(uint16 address)
{
	if (address >= MemRegisters::WavePatternBegin && address <= MemRegisters::WavePatternEnd)
	{
		return m_WavePattern[address - MemRegisters::WavePatternBegin];
	}

	static uint8 Zero = 0;

	switch(address)
	{
	case MemRegisters::NR10_CH1Sweep:
		return m_PulseA.m_CHSweep;
	case MemRegisters::NR11_CH1SoundLength:
		return m_PulseA.m_CHSoundLength;
	case MemRegisters::NR12_CH1Envelope:
		return m_PulseA.m_CHEnvelope;
	case MemRegisters::NR13_CH1FrequencyLo:
		return Zero;// NR13_CH1FrequencyLo write only
	case MemRegisters::NR14_CH1FrequencyHi:
		return m_PulseA.m_CHFrequencyHiControl;

	case MemRegisters::NR21_CH2SoundLength:
		return m_PulseB.m_CHSoundLength;
	case MemRegisters::NR22_CH2Envelope:
		return m_PulseB.m_CHEnvelope;
	case MemRegisters::NR23_CH2FrequencyLo:
		return Zero;// NR23_CH2FrequencyLo write only
	case MemRegisters::NR24_CH2FrequencyHi:
		return m_PulseB.m_CHFrequencyHiControl;

	case MemRegisters::NR30_CH3OnOff:
		return m_Wave.m_CHOnOff;
	case MemRegisters::NR31_CH3SoundLength:
		return m_Wave.m_CHSoundLength;
	case MemRegisters::NR32_CH3OutputLevel:
		return m_Wave.m_CHEnvelope;
	case MemRegisters::NR33_CH3FrequencyLo:
		return Zero; //NR33_CH3FrequencyLo write only
	case MemRegisters::NR34_CH3FrequencyHi:
		return m_Wave.m_CHFrequencyHiControl;

	case MemRegisters::NR41_CH4SoundLength:
		return m_Noise.m_CHSoundLength;
	case MemRegisters::NR42_CH4Envelope:
		return m_Noise.m_CHEnvelope;
	case MemRegisters::NR43_CH4PolyCounter:
		return m_Noise.m_CHFrequencyLo;
	case MemRegisters::NR44_CH4CounterConsecutive:
		return m_Noise.m_CHFrequencyHiControl;

	case MemRegisters::NR50_CHControl_OnOff_Volume:
		return m_NR50_CHControl_OnOff_Volume;
	case MemRegisters::NR51_SoundOutputTerminal:
		return m_NR51_SoundOutputTerminal;
	case MemRegisters::NR52_SoundOnOff:
		return m_NR52_SoundOnOff;

	default:
		break;
	}

	return Zero;
}

void GBSound::WriteMemory(uint16 address, uint8 Value)
{
	if (address >= MemRegisters::WavePatternBegin && address <= MemRegisters::WavePatternEnd)
	{
		m_WavePattern[address - MemRegisters::WavePatternBegin] = Value;
		return;
	}

	switch (address)
	{
	case MemRegisters::NR10_CH1Sweep:
		m_PulseA.m_CHSweep = Value;
		break;
	case MemRegisters::NR11_CH1SoundLength:
		m_PulseA.m_CHSoundLength = Value;
		break;
	case MemRegisters::NR12_CH1Envelope:
		m_PulseA.m_CHEnvelope = Value;
		break;
	case MemRegisters::NR13_CH1FrequencyLo:
		m_PulseA.m_CHFrequencyLo = Value;// NR13_CH1FrequencyLo write only
		break;
	case MemRegisters::NR14_CH1FrequencyHi:
		m_PulseA.m_CHFrequencyHiControl = Value;
		break;

	case MemRegisters::NR21_CH2SoundLength:
		m_PulseB.m_CHSoundLength = Value;
		break;
	case MemRegisters::NR22_CH2Envelope:
		m_PulseB.m_CHEnvelope = Value;
		break;
	case MemRegisters::NR23_CH2FrequencyLo:
		m_PulseB.m_CHFrequencyLo = Value;// NR23_CH2FrequencyLo write only
		break;
	case MemRegisters::NR24_CH2FrequencyHi:
		m_PulseB.m_CHFrequencyHiControl = Value;
		break;

	case MemRegisters::NR30_CH3OnOff:
		m_Wave.m_CHOnOff = Value;
		break;
	case MemRegisters::NR31_CH3SoundLength:
		m_Wave.m_CHSoundLength = Value;
		break;
	case MemRegisters::NR32_CH3OutputLevel:
		m_Wave.m_CHEnvelope = Value;
		break;
	case MemRegisters::NR33_CH3FrequencyLo:
		m_Wave.m_CHFrequencyLo = Value; //NR33_CH3FrequencyLo write only
		break;
	case MemRegisters::NR34_CH3FrequencyHi:
		m_Wave.m_CHFrequencyHiControl = Value;
		break;

	case MemRegisters::NR41_CH4SoundLength:
		m_Noise.m_CHSoundLength = Value;
		break;
	case MemRegisters::NR42_CH4Envelope:
		m_Noise.m_CHEnvelope = Value;
		break;
	case MemRegisters::NR43_CH4PolyCounter:
		m_Noise.m_CHFrequencyLo = Value;
		break;
	case MemRegisters::NR44_CH4CounterConsecutive:
		m_Noise.m_CHFrequencyHiControl = Value;
		break;

	case MemRegisters::NR50_CHControl_OnOff_Volume:
		m_NR50_CHControl_OnOff_Volume = Value;
		break;
	case MemRegisters::NR51_SoundOutputTerminal:
		m_NR51_SoundOutputTerminal = Value;
		break;
	case MemRegisters::NR52_SoundOnOff:
		m_NR52_SoundOnOff = Value;
		break;

	default:
		break;
	}
}

void PulseGeneric::Update(int32 Cycles)
{
	if (!IsOn() && (GetLength() == 0))
	{
		m_Output = 0.0f;
		return;
	}

	uint8 PulseWaveforms[4] ={
		0x01,
		0x81,
		0x87,
		0x7E
	};

	float timeElapsed = float(Cycles) * Timings::GBClockTime;
	m_TimeBeforeNextLenghtCheck -= timeElapsed;
	if (m_TimeBeforeNextLenghtCheck <= 0.0f)
	{
		m_TimeBeforeNextLenghtCheck += LenghtTimeCheck;

		uint8 currentLength = GetLength();
		if (currentLength > 0)
		{
			currentLength--;
			SetLength(currentLength);
		}
	}

	if ((GetLength() > 0) || (!GetCounterConsecutive()) )
	{
		float frequency = float(Timings::GBClockSpeed) / (32.0f * (2048.0f - float(GetFrequency())));
		float pulseStepLength = (1.0f / frequency) / 8.0f;
		m_CurrentSubPulseTime += timeElapsed;
		if (m_CurrentSubPulseTime >= pulseStepLength)
		{
			m_OutputBeforeVolume = 0.0f;
			m_CurrentSubPulseTime = 0.0f;
			//output the sample
			uint8 pulseRatio = GetPulseRatio();

			uint8 waveform = PulseWaveforms[pulseRatio];
			m_OutputBeforeVolume = GetBit(m_CurrentPulseStep, waveform) ? 1.0f : 0.0f;

			m_CurrentPulseStep++;
			if (m_CurrentPulseStep >= 8)
			{
				m_CurrentPulseStep = 0;
			}
		}

		//calculating envelope
		int32 currentSweepcount = GetVolumeSweepCount();
		if (currentSweepcount > 0)
		{
			m_CurrentVolumeEnvelopeTime += timeElapsed;
			if (m_CurrentVolumeEnvelopeTime >= (currentSweepcount * VolumeEnvelopeStep))
			{
				m_CurrentVolumeEnvelopeTime = 0.0f;
				if (GetVolumeRegister() > 0)
				{
					SetVolumeRegister(GetVolumeRegister() - 1);
				}
			}
		}

		m_Output = m_OutputBeforeVolume * GetVolume(Cycles);
	}
	else
	{
		m_Output = 0.0f;
	}
}

void Wave::UpdateWaveform()
{
	uint8 Shift = (m_CHEnvelope >> 5) & 0x03;
	uint8 bitShift = 0;
	switch(Shift)
	{
	case 0x00:
		bitShift = 0;
		break;
	case 0x01:
		bitShift = 0;
		break;
	case 0x02:
		bitShift = 1;
		break;
	case 0x03:
		bitShift = 2;
		break;
	default:
		break;
	}

	uint8* compressedWavePattern = m_SoundSystem->GetWavePattern();
	for (int32 i = 0; i < 16; ++i)
	{
		uint8 value = compressedWavePattern[i];
		uint8 highVal = (value >> 4) & 0x0F;
		uint8 lowVal = value & 0x0F;

		m_currentWaveform[i * 2] = highVal >> bitShift;
		m_currentWaveform[i * 2 + 1] = lowVal >> bitShift;
	}
}

void Wave::Update(int32 Cycles)
{
	if (!IsOn() && (GetLength() == 0))
	{
		m_Output = 0.0f;
		return;
	}

	UpdateWaveform();

	float timeElapsed = float(Cycles) * Timings::GBClockTime;
	m_TimeBeforeNextLenghtCheck -= timeElapsed;
	if (m_TimeBeforeNextLenghtCheck <= 0.0f)
	{
		m_TimeBeforeNextLenghtCheck += LenghtTimeCheck;

		uint8 currentLength = GetLength();
		if (currentLength > 0)
		{
			currentLength--;
			SetLength(currentLength);
		}
	}

	if ((GetLength() > 0) || (!GetCounterConsecutive()))
	{
		/*float frequency = float(Timings::GBClockSpeed) / (64.0f * (2048.0f - float(GetFrequency())));
		float sampleStepLength = (1.0f / frequency) / 32.0f;
		m_CurrentSampleTime += timeElapsed;
		if (m_CurrentSampleTime >= sampleStepLength)
		{
			m_CurrentSampleTime = 0.0f;
			m_Output = (float(m_currentWaveform[m_CurrentSample]) / 16.0f) * GetVolume(Cycles);

			m_CurrentSample++;
			if (m_CurrentSample >= 32)
			{
				m_CurrentSample = 0;
			}
		}*/

		int32 frequencyCycles = (64.0f * (2048.0f - float(GetFrequency()))) / 32;
		m_CurrentSampleTimeCycles += Cycles;
		if (m_CurrentSampleTimeCycles >= frequencyCycles)
		{
			m_CurrentSampleTimeCycles -= frequencyCycles;
			m_Output = (float(m_currentWaveform[m_CurrentSample]) / 16.0f) * GetVolume(Cycles);

			m_CurrentSample++;
			if (m_CurrentSample >= 32)
			{
				m_CurrentSample = 0;
			}
		}
	}
	else
	{
		m_Output = 0.0f;
	}
}

bool Noise::Get15or7Steps()
{
	return GetBit(3, m_CHFrequencyLo);
}

float Noise::GetClockDivider()
{
	uint8 dividerVal = m_CHFrequencyLo & 0x07;
	float f = float(Timings::GBClockSpeed) / 8.0f;
	switch (dividerVal)
	{
	case 0:
		return f * 2.0f;
	case 1:
		return f;
	case 2: 
		return f / 2.0f;
	case 3:
		return f / 3.0f;
	case 4:
		return f / 4.0f;
	case 5:
		return f / 5.0f;
	case 6: 
		return f / 6.0f;
	case 7:
		return f / 7.0f;
	default:
		break;
	}

	return 1.0f;
}

float Noise::GetPreScalerDivider()
{
	uint8 prescalerVal = (m_CHFrequencyLo >> 4) & 0x0F;
	return GetClockDivider() / pow(2.0f, float(prescalerVal + 1.0f));
}

bool Noise::ShiftRegister()
{
	uint8 outBit = GetBit(0, m_shiftRegister) ? 0x01 : 0x00;
	uint8 bit1 = GetBit(1, m_shiftRegister) ? 0x01 : 0x00;
	uint8 newLeftmost = outBit ^ bit1;
	m_shiftRegister = m_shiftRegister >> 1;
	m_shiftRegister = SetBit(14, m_shiftRegister, GetBit(0, newLeftmost));
	if (Get15or7Steps())
	{
		//7
		m_shiftRegister = SetBit(6, m_shiftRegister, GetBit(0, newLeftmost));
	}

	return !outBit; //the bit inverted
}

void Noise::Update(int32 Cycles)
{
	if (!IsOn() && (GetLength() == 0))
	{
		m_Output = 0.0f;
		return;
	}

	float timeElapsed = float(Cycles) * Timings::GBClockTime;
	m_TimeBeforeNextLenghtCheck -= timeElapsed;
	if (m_TimeBeforeNextLenghtCheck <= 0.0f)
	{
		m_TimeBeforeNextLenghtCheck += LenghtTimeCheck;

		uint8 currentLength = GetLength();
		if (currentLength > 0)
		{
			currentLength--;
			SetLength(currentLength);
		}
	}

	if ((GetLength() > 0) || (!GetCounterConsecutive()))
	{
		float sampleLength = 1.0f / GetPreScalerDivider();
		m_CurrentSampleTime -= timeElapsed;
		if (m_CurrentSampleTime <= 0.0f)
		{
			m_CurrentSampleTime = sampleLength;

			m_OutputBeforeVolume= ShiftRegister() ? 1.0f : 0.0f;
		}

		//calculating envelope
		int32 currentSweepcount = GetVolumeSweepCount();
		if (currentSweepcount > 0)
		{
			m_CurrentVolumeEnvelopeTime += timeElapsed;
			if (m_CurrentVolumeEnvelopeTime >= (currentSweepcount * VolumeEnvelopeStep))
			{
				m_CurrentVolumeEnvelopeTime = 0.0f;
				if (GetVolumeRegister() > 0)
				{
					SetVolumeRegister(GetVolumeRegister() - 1);
				}
			}
		}

		m_Output = m_OutputBeforeVolume * GetVolume(Cycles);
	}
	else
	{
		m_Output = 0.0f;
	}
}

void GBSound::GetChannelVolumes(float& Left, float& Right)
{
	Left = float(m_NR50_CHControl_OnOff_Volume & 0x07) / 7.0f;
	Right = float((m_NR50_CHControl_OnOff_Volume >> 4) & 0x07) / 7.0f;
}

void GBSound::GetTerminalFromChannel(int32 Channel, bool& Left, bool& Right)
{
	uint8 leftBit = Channel - 1;
	uint8 rightBit = Channel + 3;
	Left = GetBit(leftBit, m_NR51_SoundOutputTerminal);
	Right = GetBit(rightBit, m_NR51_SoundOutputTerminal);
}

bool GBSound::IsSoundOn()
{
	return GetBit(7, m_NR52_SoundOnOff);
}

void GBSound::MixChannel(const SoundChannel& Channel, int32 Number, float& Left, float& Right)
{
	if (!IsSoundOn())
	{
		Left = 0.0f;
		Right = 0.0f;
		return;
	}

	float sample = Channel.GetOutput();
	bool goesLeft;
	bool goesRight;
	GetTerminalFromChannel(Number, goesLeft, goesRight);

	if (goesLeft)
	{
		Left += sample * 0.25f; // so 4 channels at full power makes 1
	}

	if (goesRight)
	{
		Right += sample * 0.25f;
	}
}

void GBSound::Update(int32 Cycles)
{
	m_PulseA.Update(Cycles);
	m_PulseB.Update(Cycles);
	m_Wave.Update(Cycles);
	m_Noise.Update(Cycles);

	//Sound update
	m_CurrentCyclesCount += Cycles;
	if (m_CurrentCyclesCount >= CyclesPerSample)
	{
		m_CurrentCyclesCount = 0;
		//output the sample

		float sampleLeft = 0.0f;
		float sampleRight = 0.0f;

		MixChannel(m_PulseA, 1, sampleLeft, sampleRight);
		MixChannel(m_PulseB, 2, sampleLeft, sampleRight);
		MixChannel(m_Wave, 3, sampleLeft, sampleRight);
		MixChannel(m_Noise, 4, sampleLeft, sampleRight);

		float leftVolume = 1.0f;
		float rightVolume = 1.0f;
		GetChannelVolumes(leftVolume, rightVolume);

		m_GeneratedSamples[m_CurrentSample].m_Left = sampleLeft * leftVolume;
		m_GeneratedSamples[m_CurrentSample].m_Right = sampleRight * rightVolume;

		m_CurrentSample++;
		if (m_CurrentSample >= BufferSize)
		{
			SDL_QueueAudio(m_Device, m_GeneratedSamples, BufferSize * sizeof(SoundSample));
			m_CurrentSample = 0;
		}
	}
}