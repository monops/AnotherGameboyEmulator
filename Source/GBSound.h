#pragma once

#include "Types.h"
#include "MemoryElement.h"
#include "SDL.h"
#include "Constants.h"

class SoundChannel
{
public:

	SoundChannel(class GBSound* SoundSystem);

	uint8 m_CHSoundLength;
	uint8 m_CHEnvelope;
	uint8 m_CHFrequencyLo;
	uint8 m_CHFrequencyHiControl;

	virtual bool IsOn();
	virtual void Update(int32 Cycles) {};
	virtual float GetVolume(int32 Cycles);
	virtual uint8 GetVolumeRegister();
	virtual float GetEnvelope(int32 Cycles);
	virtual void SetVolumeRegister(uint8 volume);

	virtual bool GetVolumeSweepDirection();
	virtual int32 GetVolumeSweepCount();
	virtual void SetVolumeSweepCount(int32 count);

	virtual uint16 GetFrequency();
	virtual void SetFrequency(uint16 Frequency);

	virtual uint8 GetLength();
	virtual void SetLength(uint8 newLength);

	bool GetCounterConsecutive();

	float GetOutput() const { return m_Output; }

protected:
	static constexpr float VolumeEnvelopeStep = 1.0f / 64.0f;

	float m_CurrentVolumeEnvelopeTime = 0.0f;
	class GBSound* m_SoundSystem;
	float m_Output = 0.0f;
};

class PulseGeneric : public SoundChannel
{
public:
	PulseGeneric(class GBSound* SoundSystem): SoundChannel(SoundSystem)
	{}

	static constexpr float LenghtTimeCheck = 1.0f / 256.0f;

	uint8 GetPulseRatio();

	virtual void Update(int32 Cycles) override;
protected:
	float m_TimeBeforeNextLenghtCheck = 0.0f;
	int32 m_CurrentPulseStep = 0; // max 7;
	float m_CurrentSubPulseTime = 0.0f;

	float m_OutputBeforeVolume = 0.0f;
};

class PulseA : public PulseGeneric
{
public:
	PulseA(class GBSound* SoundSystem) : PulseGeneric(SoundSystem)
	{}

	uint8 m_CHSweep;

	int32 GetFrequencySweepShiftCount();
	bool GetFrequenctSweepDirection();
	int32 GetFrequencySweepTime();
	void SetFrequencyShiftCount(uint8 newCount);

	virtual void Update(int32 Cycles) override;

protected:
	float m_currentSweepTime = 0.0f;
};

class PulseB : public PulseGeneric
{
public:
	PulseB(class GBSound* SoundSystem) : PulseGeneric(SoundSystem)
	{}

};

class Wave : public SoundChannel
{
public:
	static constexpr float LenghtTimeCheck = 1.0f / 256.0f;

	Wave(class GBSound* SoundSystem) : SoundChannel(SoundSystem)
	{}

	uint8 m_CHOnOff;

	virtual bool IsOn() override;
	virtual float GetVolume(int32 Cycles) override;
	virtual bool GetVolumeSweepDirection() override { return false; }
	virtual int32 GetVolumeSweepCount() override { return 0; }
	virtual uint8 GetLength() override
	{
		return m_CHSoundLength;
	}

	virtual void Update(int32 Cycles) override;

protected:
	uint8 m_currentWaveform[32];
	void UpdateWaveform();

	float m_TimeBeforeNextLenghtCheck = 0.0f;
	int32 m_CurrentSample = 0; // max 32;
	float m_CurrentSampleTime = 0.0f;
	int32 m_CurrentSampleTimeCycles = 0;
};

class Noise : public SoundChannel
{
public:
	static constexpr float LenghtTimeCheck = 1.0f / 256.0f;

	Noise(class GBSound* SoundSystem) : SoundChannel(SoundSystem)
	{}

	bool Get15or7Steps();
	float GetClockDivider();
	float GetPreScalerDivider();

	bool ShiftRegister();

	virtual void Update(int32 Cycles) override;

protected:
	float m_TimeBeforeNextLenghtCheck = 0.0f;
	float m_CurrentSampleTime = 0.0f;
	uint16 m_shiftRegister = 0xFFFF;
	float m_OutputBeforeVolume = 0.0f;
};

class GBSound : public IMemoryElement
{
public:
	struct SoundSample
	{
		float m_Left;
		float m_Right;
	};

	static constexpr float RequestedBufferTime = 1.0f / 60.0f;

	static constexpr uint32 Frequency = 44100;
	static constexpr float SampleLength = 1.0f / float(Frequency);
	static constexpr uint32 BufferSize = 1024;// uint32(RequestedBufferTime / SampleLength);
	static constexpr uint32 CyclesPerSample = uint32(Timings::GBClockSpeed * SampleLength);

	GBSound(class GameBoyCPU* InCPU);
	~GBSound();

	virtual uint8& ReadMemory(uint16 address) override;
	virtual void WriteMemory(uint16 address, uint8 Value) override;

	void Update(int32 Cycles);
	uint8* GetWavePattern() { return m_WavePattern; }

	void GetChannelVolumes(float& Left, float& Right);
	void GetTerminalFromChannel(int32 Channel, bool& Left, bool& Right);
	void MixChannel(const SoundChannel& Channel, int32 Number, float& Left, float& Right);
	bool IsSoundOn();

private:

	class GameBoyCPU* CPU;

	uint8 m_WavePattern[16];
	uint8 m_NR50_CHControl_OnOff_Volume;
	uint8 m_NR51_SoundOutputTerminal;
	uint8 m_NR52_SoundOnOff = 0xF1;

	PulseA m_PulseA;
	PulseB m_PulseB;
	Wave m_Wave;
	Noise m_Noise;

	//sound output
	SDL_AudioDeviceID m_Device;
	SoundSample m_GeneratedSamples[BufferSize]; // just to be sure to not overrun
	uint32 m_CurrentCyclesCount = 0;
	uint32 m_CurrentSample = 0;

	static void AudioCallback(void*  userdata,
		Uint8* stream,
		int    len);
};