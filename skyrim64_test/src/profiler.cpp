#include "common.h"

#if SKYRIM64_USE_VTUNE && SKYRIM64_USE_PROFILER
#pragma message("Warning: Using the built-in profiler code with VTune may skew final results")
#endif

#if SKYRIM64_USE_PROFILER
namespace Profiler
{
    namespace Internal
    {
        std::array<Entry, MaxEntries> GlobalCounters;
        std::unordered_map<uint32_t, Entry *> LookupMap;
        int64_t QpcFrequency;
		int64_t CpuFrequency;

		void ReadCounters(int64_t& TSC, int64_t& QPC)
		{
			uint32_t unused;
			TSC = __rdtscp(&unused);

			LARGE_INTEGER p;
			QueryPerformanceCounter(&p);
			QPC = p.QuadPart;
		}

		void CalibrateQPC()
		{
			QueryPerformanceFrequency((LARGE_INTEGER *)&QpcFrequency);
		}

		void CalibrateRDTSC()
		{
			//
			// Modified from: https://github.com/benvanik/xenia/issues/801#issuecomment-352202912
			//

			// Try to compute the RDTSC update frequency with the highest thread priority on a single core
			DWORD_PTR oldAffinity = SetThreadAffinityMask(GetCurrentThread(), 1ull << GetCurrentProcessorNumber());
			int oldPriority = GetThreadPriority(GetCurrentThread());

			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

			// Loop multiple times and average it. Discard any results where the interval is out of reasonable range.
			{
				double perfCounterFactor = 1000000.0 / (double)QpcFrequency;
				double frequency = 0.0;

				int64_t tscA;
				int64_t pcA;

				int64_t tscB;
				int64_t pcB;

				double interval;

				int times = 64;
				int retries = 10;

				for (int i = 0; i < times; i++)
				{
					for (int j = 0; j < retries; j++)
					{
						ReadCounters(tscA, pcA);

						do
						{
							ReadCounters(tscB, pcB);
							interval = double(pcB - pcA) * perfCounterFactor;
						} while (interval < 1000.0);

						if (interval < 1001.0)
						{
							frequency += double(tscB - tscA) / interval;
							break;
						}
					}
				}

				// Calculate average, then convert MHz to Hz
				CpuFrequency = int64_t(ceil(frequency / double(times)));
				CpuFrequency *= 1000000;
			}

			SetThreadPriority(GetCurrentThread(), oldPriority);
			SetThreadAffinityMask(GetCurrentThread(), oldAffinity);
		}

		STATIC_CONSTRUCTOR(__profiler, []
		{
			CalibrateQPC();
			CalibrateRDTSC();
		});

        Entry *FindEntry(uint32_t CRC)
        {
            // Check if it's in the hashmap
            auto entry = Internal::LookupMap[CRC];

            if (entry)
                return entry;

            // Otherwise check the whole array, then add it to the map
            for (auto &counter : Internal::GlobalCounters)
            {
                if (!counter.Init)
                    continue;

                if (Internal::CRC32(counter.Name) != CRC)
                    continue;

                Internal::LookupMap[CRC] = &counter;
                return &counter;
            }

            return nullptr;
        }
    }

    int64_t GetValue(uint32_t CRC)
    {
        if (auto e = Internal::FindEntry(CRC); e)
        {
            // e->Value might be updated in the middle of this code
            int64_t temp = e->Value;
            e->OldValue  = temp;
            return temp;
        }

        return 0;
    }

	int64_t GetDeltaValue(uint32_t CRC)
	{
		if (auto e = Internal::FindEntry(CRC); e)
			return e->Value - e->OldValue;

		return 0;
	}

    double GetTime(uint32_t CRC)
    {
		return ((double)GetValue(CRC) / (double)Internal::CpuFrequency) * 1000.0;
    }

	double GetDeltaTime(uint32_t CRC)
	{
		return ((double)GetDeltaValue(CRC) / (double)Internal::CpuFrequency) * 1000.0;
	}
}
#endif // SKYRIM64_USE_PROFILER