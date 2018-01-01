#pragma once

#include <windows.h>
#include <intrin.h>

class BSSpinLock
{
private:
	const static uint32_t SLOW_PATH_BACKOFF_COUNT = 10000;

	uint32_t m_OwningThread;
	volatile uint32_t m_LockCount;

public:
	BSSpinLock();

	void Acquire(int InitialAttemps = 0);
	void Release();

	bool IsLocked();
	bool ThreadOwnsLock();
};