#include "stdafx.h"

#define PROCESS_CALLBACK_FILTER_ENABLED 0x1

BOOL (WINAPI * SetProcessUserModeExceptionPolicyPtr)(DWORD PolicyFlags);
BOOL (WINAPI * GetProcessUserModeExceptionPolicyPtr)(LPDWORD PolicyFlags);

void DisableDumpBreakpoint();
BOOL WINAPI hk_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
{
	ULONG_PTR ret = (ULONG_PTR)_ReturnAddress();

	if (ret >= g_ModuleBase && ret <= (g_ModuleBase + g_ModuleSize))
	{
		char buf[128];
		sprintf_s(buf, "dump SkyrimSE.exe || ret: 0x%llX", ret);
		MessageBoxA(nullptr, buf, "", 0);

		HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

		if (hKernel32)
		{
			*(FARPROC*)&SetProcessUserModeExceptionPolicyPtr = GetProcAddress(hKernel32, "SetProcessUserModeExceptionPolicy");
			*(FARPROC*)&GetProcessUserModeExceptionPolicyPtr = GetProcAddress(hKernel32, "GetProcessUserModeExceptionPolicy");
		}

		// Ensure that exceptions are not swallowed when dispatching certain Windows
		// messages.
		//
		// http://blog.paulbetts.org/index.php/2010/07/20/the-case-of-the-disappearing-onload-exception-user-mode-callback-exceptions-in-x64/
		// https://support.microsoft.com/en-gb/kb/976038
		if (SetProcessUserModeExceptionPolicyPtr && GetProcessUserModeExceptionPolicyPtr)
		{
			DWORD flags = 0;

			if (GetProcessUserModeExceptionPolicyPtr(&flags))
				SetProcessUserModeExceptionPolicyPtr(flags & ~PROCESS_CALLBACK_FILTER_ENABLED);
		}

		DWORD oldMode = 0;
		SetErrorMode(0);
		SetThreadErrorMode(0, &oldMode);

		DisableDumpBreakpoint();
	}

	return QueryPerformanceCounter(lpPerformanceCount);
}

void DisableDumpBreakpoint()
{
	// Restore the original pointer
	PatchIAT(QueryPerformanceCounter, "kernel32.dll", "QueryPerformanceCounter");
}

void EnableDumpBreakpoint()
{
	PatchIAT(hk_QueryPerformanceCounter, "kernel32.dll", "QueryPerformanceCounter");
}