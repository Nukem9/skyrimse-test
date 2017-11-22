#pragma once

#include <windows.h>
#include <intrin.h>

class BSSpinLock
{
private:
	const uint32_t SLOW_PATH_BACKOFF_COUNT = 10000;

	uint32_t m_OwnerThreadId;
	volatile uint32_t m_LockCount;

public:
	BSSpinLock();

	void Acquire(int InitialAttemps = 0);
	void Release();

	bool IsLocked();
	bool ThreadOwnsLock();
};