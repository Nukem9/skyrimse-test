#pragma once

#include <stdint.h>
#include <array>
#include <unordered_map>

#if !SKYRIM64_USE_PROFILER
#define ProfileCounterInc(Name)			((void)0)
#define ProfileCounterAdd(Name, Add)	((void)0)
#define ProfileTimer(Name)				((void)0)

#define ProfileGetValue(Name)			(0)
#define ProfileGetDeltaValue(Name)		(0)
#define ProfileGetTime(Name)			(0.0)
#define ProfileGetDeltaTime(Name)		(0.0)
#else
#define EXPAND_MACRO(x) x
#define LINEID EXPAND_MACRO(__z)__COUNTER__

#define ProfileCounterInc(Name)			Profiler::ScopedCounter<Profiler::Internal::CRC32Index(Name)> LINEID(__FILE__, __FUNCTION__, Name);
#define ProfileCounterAdd(Name, Add)	Profiler::ScopedCounter<Profiler::Internal::CRC32Index(Name)> LINEID(__FILE__, __FUNCTION__, Name, Add);
#define ProfileTimer(Name)				Profiler::ScopedTimer<Profiler::Internal::CRC32Index(Name)> LINEID(__FILE__, __FUNCTION__, Name);

#define ProfileGetValue(Name)			Profiler::GetValue<Profiler::Internal::CRC32(Name)>()
#define ProfileGetDeltaValue(Name)		Profiler::GetDeltaValue<Profiler::Internal::CRC32(Name)>()
#define ProfileGetTime(Name)			Profiler::GetTime<Profiler::Internal::CRC32(Name)>()
#define ProfileGetDeltaTime(Name)		Profiler::GetDeltaTime<Profiler::Internal::CRC32(Name)>()
#endif

namespace Profiler
{
	namespace Internal
	{
#include "profiler_internal.h"
	}

	template<uint32_t UniqueIndex>
	class ScopedCounter
	{
	private:
		static_assert(UniqueIndex < Internal::MaxEntries, "Increase max array size");

		ScopedCounter() = delete;
		ScopedCounter(ScopedCounter&) = delete;

	public:
		inline ScopedCounter(const char *File, const char *Function, const char *Name)
		{
			if (!m_Entry.Init)
				m_Entry = { 0, 0, File, Function, Name, true };

			InterlockedIncrement64(&m_Entry.Value);
		}

		inline ScopedCounter(const char *File, const char *Function, const char *Name, int64_t Add)
		{
			if (!m_Entry.Init)
				m_Entry = { 0, 0, File, Function, Name, true };

			InterlockedAdd64(&m_Entry.Value, Add);
		}

	private:
		Internal::Entry& m_Entry = Internal::GlobalCounters[UniqueIndex];
	};

	template<uint32_t UniqueIndex>
	class ScopedTimer
	{
	private:
		static_assert(UniqueIndex < Internal::MaxEntries, "Increase max array size");

		ScopedTimer() = delete;
		ScopedTimer(ScopedTimer&) = delete;

		__forceinline void GetTime(LARGE_INTEGER *Counter)
		{
#if 1
			uint32_t temp;
			Counter->QuadPart = __rdtscp(&temp);
#else
			QueryPerformanceCounter(Counter);
#endif
		}

	public:
		__forceinline ScopedTimer(const char *File, const char *Function, const char *Name)
		{
			if (!m_Entry.Init)
				m_Entry = { 0, 0, File, Function, Name, true };

			GetTime(&m_Start);
		}

		__forceinline ~ScopedTimer()
		{
			LARGE_INTEGER endTime;
			GetTime(&endTime);

			InterlockedAdd64(&m_Entry.Value, endTime.QuadPart - m_Start.QuadPart);
		}

	private:
		Internal::Entry& m_Entry = Internal::GlobalCounters[UniqueIndex];
		LARGE_INTEGER m_Start;
	};

	int64_t GetValue(uint32_t CRC);
	int64_t GetDeltaValue(uint32_t CRC);
	double GetTime(uint32_t CRC);
	double GetDeltaTime(uint32_t CRC);

	template<uint32_t CRC>
	int64_t GetValue()
	{
		return GetValue(CRC);
	}

	template<uint32_t CRC>
	int64_t GetDeltaValue()
	{
		return GetDeltaValue(CRC);
	}

	template<uint32_t CRC>
	double GetTime()
	{
		return GetTime(CRC);
	}

	template<uint32_t CRC>
	double GetDeltaTime()
	{
		return GetDeltaTime(CRC);
	}

	float GetProcessorUsagePercent();
	float GetThreadUsagePercent();
	float GetGpuUsagePercent(int GpuIndex = 0);
}