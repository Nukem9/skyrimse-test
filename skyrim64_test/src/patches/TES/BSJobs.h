#pragma once

#include <atomic>
#include <unordered_map>

class BSJobs
{
public:
	struct TrackingInfo
	{
		std::string Name;
		std::atomic_uint64_t TotalCount;	// Total number of invocations
		std::atomic_uint32_t ActiveCount;	// Currently running # of instances

		TrackingInfo& operator=(const TrackingInfo& Other)
		{
			Name = Other.Name;
			TotalCount = Other.TotalCount.load();
			ActiveCount = Other.ActiveCount.load();
			return *this;
		}
	};

	const static std::unordered_map<uintptr_t, std::string> JobNameMap;
	static std::unordered_map<uintptr_t, BSJobs::TrackingInfo> JobTracker;

	static void DispatchJobCallback(void *Parameter, void(*Function)(void *));
};