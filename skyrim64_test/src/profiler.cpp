#include "common.h"

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

			// Try to minimize timer update resolution
			timeBeginPeriod(0);

			// Try to compute the RDTSC update frequency with the highest thread priority on a single core
			DWORD_PTR oldAffinity = SetThreadAffinityMask(GetCurrentThread(), 1ull << GetCurrentProcessorNumber());
			int oldPriority = GetThreadPriority(GetCurrentThread());

			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

			// Loop multiple times and average it. Discard any results where the interval is out of reasonable range.
			{
				double perfCounterFactor = 1000000.0 / (double)QpcFrequency;
				double frequency = 0.0;

				int times = 64;
				int retries = 10;
				double interval;

				int64_t pca;
				int64_t pcb;
				int64_t tsca;
				int64_t tscb;

				for (int j = 0; j < times; ++j)
				{
					for (int i = 0; i < retries; ++i)
					{
						ReadCounters(tsca, pca);

						do
						{
							ReadCounters(tscb, pcb);
							interval = double(pcb - pca) * perfCounterFactor;
						} while (interval < 1000.0);

						if (interval < 1001.0)
						{
							frequency += double(tscb - tsca) / interval;
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
        return (double)GetValue(CRC) / (double)Internal::CpuFrequency;
    }

	double GetDeltaTime(uint32_t CRC)
	{
		return (double)GetDeltaValue(CRC) / (double)Internal::CpuFrequency;
	}
}
#endif // SKYRIM64_USE_PROFILER
