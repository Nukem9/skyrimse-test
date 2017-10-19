#pragma once

#include <array>
#include <unordered_map>

#define PROFILER_ENABLED

#ifdef PROFILER_ENABLED
#define WTFTEST(x) x

#ifdef _DEBUG
#define LINEID WTFTEST(__z)__COUNTER__
#else
#define LINEID WTFTEST(__z)__LINE__
#endif

#define ProfileCounterInc(Name)			Profiler::ScopedCounter<Profiler::Internal::CRC32Index(Name)> LINEID(__FILE__, __FUNCTION__, Name);
#define ProfileCounterAdd(Name, Add)	Profiler::ScopedCounter<Profiler::Internal::CRC32Index(Name)> LINEID(__FILE__, __FUNCTION__, Name, Add);
#define ProfileTimer(Name)				Profiler::ScopedTimer<Profiler::Internal::CRC32Index(Name)> LINEID(__FILE__, __FUNCTION__, Name);

#define ProfileGetValue(Name)			Profiler::GetValue<Profiler::Internal::CRC32(Name)>()
#define ProfileGetDeltaValue(Name)		Profiler::GetDeltaValue<Profiler::Internal::CRC32(Name)>()
//#define ProfileGetDouble(Name)			((double)Profiler::GetValue<Profiler::Internal::CRC32(Name)>())
#define ProfileGetTime(Name)			Profiler::GetTime<Profiler::Internal::CRC32(Name)>()
#define ProfileGetDeltaTime(Name)		Profiler::GetDeltaTime<Profiler::Internal::CRC32(Name)>()
#else
#define ProfileCounterInc(Name)
#define ProfileCounterAdd(Name, Add)
#define ProfileTimer(Name)

#define ProfileGetValue(Name)			(0)
//#define ProfileGetDouble(Name)			(0.0)
#define ProfileGetTime(Name)			(0.0)
#endif

#ifdef PROFILER_ENABLED
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

	public:
		inline ScopedTimer(const char *File, const char *Function, const char *Name)
		{
			if (!m_Entry.Init)
				m_Entry = { 0, 0, File, Function, Name, true };

			QueryPerformanceCounter(&m_Start);
		}

		inline ~ScopedTimer()
		{
			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);

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
}
#endif // PROFILER_ENABLED