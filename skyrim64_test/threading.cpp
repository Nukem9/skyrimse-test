#include "stdafx.h"

// Affinity hooks that probably don't do anything

BOOL WINAPI hk_SetThreadPriority(HANDLE hThread, int nPriority)
{
	// Do not allow a priority that is below normal
	nPriority = max(THREAD_PRIORITY_NORMAL, nPriority);

	return SetThreadPriority(hThread, nPriority);
}

DWORD_PTR WINAPI hk_SetThreadAffinityMask(HANDLE hThread, DWORD_PTR dwThreadAffinityMask)
{
	// Two calls must be made because of possible GetLastError() checking
	DWORD_PTR result = SetThreadAffinityMask(hThread, dwThreadAffinityMask);

	if (result == 0)
		return result;

	return SetThreadAffinityMask(hThread, result);
}

void PatchThreading()
{
	PatchIAT(hk_SetThreadPriority, "kernel32.dll", "SetThreadPriority");
	PatchIAT(hk_SetThreadAffinityMask, "kernel32.dll", "SetThreadAffinityMask");

	timeBeginPeriod(1);
}