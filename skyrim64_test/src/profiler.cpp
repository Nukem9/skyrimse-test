#include "common.h"

namespace Profiler
{
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34

	typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
	typedef int(*NvAPI_Initialize_t)();
	typedef int(*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);
	typedef int(*NvAPI_GPU_GetUsages_t)(int *handle, unsigned int *usages);

	NvAPI_QueryInterface_t      NvAPI_QueryInterface = NULL;
	NvAPI_Initialize_t          NvAPI_Initialize = NULL;
	NvAPI_EnumPhysicalGPUs_t    NvAPI_EnumPhysicalGPUs = NULL;
	NvAPI_GPU_GetUsages_t       NvAPI_GPU_GetUsages = NULL;

	int          gpuCount = 0;
	int         *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
	unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };

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

			HMODULE hmod = LoadLibraryA("nvapi64.dll");
			if (hmod == NULL)
				__debugbreak();

			// nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
			NvAPI_QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(hmod, "nvapi_QueryInterface");

			// some useful internal functions that aren't exported by nvapi.dll
			NvAPI_Initialize = (NvAPI_Initialize_t)(*NvAPI_QueryInterface)(0x0150E828);
			NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t)(*NvAPI_QueryInterface)(0xE5AC921F);
			NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t)(*NvAPI_QueryInterface)(0x189A1FDF);

			if (NvAPI_Initialize == NULL || NvAPI_EnumPhysicalGPUs == NULL ||
				NvAPI_EnumPhysicalGPUs == NULL || NvAPI_GPU_GetUsages == NULL)
			{
				__debugbreak();
			}

			// initialize NvAPI library, call it once before calling any other NvAPI functions
			(*NvAPI_Initialize)();

			(*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);
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

	uint64_t GetSystemCpuTime()
	{
		FILETIME sysKernelTime;
		FILETIME sysUserTime;
		GetSystemTimes(nullptr, &sysKernelTime, &sysUserTime);

		uint64_t sysTime =
			(((uint64_t)sysKernelTime.dwHighDateTime << 32) | sysKernelTime.dwLowDateTime) +
			(((uint64_t)sysUserTime.dwHighDateTime << 32) | sysUserTime.dwLowDateTime);

		return sysTime;
	}

	float GetProcessorUsagePercent()
	{
		thread_local uint64_t previousProcessTime;
		thread_local uint64_t previousSystemTime;
		thread_local float previousPercentage;
		
		FILETIME unused;
		FILETIME procKernelTime;
		FILETIME procUserTime;
		GetProcessTimes(GetCurrentProcess(), &unused, &unused, &procKernelTime, &procUserTime);

		uint64_t procTime =
			(((uint64_t)procKernelTime.dwHighDateTime << 32) | procKernelTime.dwLowDateTime) +
			(((uint64_t)procUserTime.dwHighDateTime << 32) | procUserTime.dwLowDateTime);

		uint64_t sysTime = GetSystemCpuTime();

		uint64_t deltaProc = procTime - previousProcessTime;
		uint64_t deltaSys = sysTime - previousSystemTime;

		previousProcessTime = procTime;
		previousSystemTime = sysTime;

		// Temp (percentage) must be cached because the timers have a variable 1ms - 20ms resolution
		float temp;

		if (deltaProc == 0 || deltaSys == 0)
			temp = 0.0;
		else
			temp = (float)deltaProc / (float)deltaSys;

		if (temp <= 0.001f)
			temp = previousPercentage;
		else
			previousPercentage = temp;

		return temp * 100.0f;
	}

	float GetThreadUsagePercent()
	{
		thread_local uint64_t previousThreadTime;
		thread_local uint64_t previousSystemTime;
		thread_local float previousPercentage;

		FILETIME unused;
		FILETIME threadKernelTime;
		FILETIME threadUserTime;
		GetThreadTimes(GetCurrentThread(), &unused, &unused, &threadKernelTime, &threadUserTime);

		uint64_t threadTime =
			(((uint64_t)threadKernelTime.dwHighDateTime << 32) | threadKernelTime.dwLowDateTime) +
			(((uint64_t)threadUserTime.dwHighDateTime << 32) | threadUserTime.dwLowDateTime);

		uint64_t sysTime = GetSystemCpuTime();

		uint64_t deltaThread = threadTime - previousThreadTime;
		uint64_t deltaSys = sysTime - previousSystemTime;

		previousThreadTime = threadTime;
		previousSystemTime = sysTime;

		// Temp (percentage) must be cached because the timers have a variable 1ms - 20ms resolution
		float temp;

		if (deltaThread == 0 || deltaSys == 0)
			temp = 0.0;
		else
			temp = (float)deltaThread / (float)deltaSys;

		if (temp <= 0.001f)
			temp = previousPercentage;
		else
			previousPercentage = temp;

		return temp * 100.0f;
	}

	float GetGpuUsagePercent(int GpuIndex)
	{
		// gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
		gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;

		(*NvAPI_GPU_GetUsages)(gpuHandles[GpuIndex], gpuUsages);
		int usage = gpuUsages[3];

		return (float)usage;
	}
}