#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class Timer
{
public:
	Timer()
	{

	}

	static void InitTimer()
	{
		LARGE_INTEGER IntFrequency;
		QueryPerformanceFrequency(&IntFrequency);
		SecondsPerCycle = 1.0 / IntFrequency.QuadPart;
	}

	void Start()
	{
		QueryPerformanceCounter(&StartTime);
	}

	double End()
	{
		LARGE_INTEGER EndTimer;
		QueryPerformanceCounter(&EndTimer);

		return (double(EndTimer.QuadPart)  - double(StartTime.QuadPart))* SecondsPerCycle;
	}

private:
	static double SecondsPerCycle;
	LARGE_INTEGER StartTime;
};