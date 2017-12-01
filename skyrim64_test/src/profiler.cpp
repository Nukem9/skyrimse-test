#include "common.h"

#if SKYRIM64_USE_PROFILER
namespace Profiler
{
    namespace Internal
    {
        std::array<Entry, MaxEntries> GlobalCounters;
        std::unordered_map<uint32_t, Entry *> LookupMap;
        int64_t CpuFrequency;

        STATIC_CONSTRUCTOR(__profiler, []
		{
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			CpuFrequency = freq.QuadPart;
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
