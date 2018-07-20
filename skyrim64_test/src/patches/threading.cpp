#include "../common.h"

// Hooks that probably don't do anything

BOOL WINAPI hk_SetThreadPriority(HANDLE hThread, int nPriority)
{
	// Don't allow a priority below normal - Skyrim doesn't have many "idle" threads
	return SetThreadPriority(hThread, std::max(THREAD_PRIORITY_NORMAL, nPriority));
}

DWORD_PTR WINAPI hk_SetThreadAffinityMask(HANDLE hThread, DWORD_PTR dwThreadAffinityMask)
{
	// Don't change anything
	return 0xFFFFFFFF;
}

void WINAPI hk_Sleep(DWORD dwMilliseconds)
{
	// Bethesda's spinlock calls Sleep(0) every iteration until 10,000. Then it
	// uses Sleep(1). Even with 0ms waits, there's a tiny performance penalty.
	if (dwMilliseconds == 0)
		return;

	SleepEx(dwMilliseconds, FALSE);
}

void PatchThreading()
{
	PatchIAT(hk_SetThreadPriority, "kernel32.dll", "SetThreadPriority");
	PatchIAT(hk_SetThreadAffinityMask, "kernel32.dll", "SetThreadAffinityMask");
	PatchIAT(hk_Sleep, "kernel32.dll", "Sleep");

	SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
	timeBeginPeriod(1);
}