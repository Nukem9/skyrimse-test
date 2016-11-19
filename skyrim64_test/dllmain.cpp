#include "stdafx.h"

ULONG_PTR g_ModuleBase;
ULONG_PTR g_ModuleSize;

ULONG_PTR g_CodeBase;
ULONG_PTR g_CodeSize;

#define PROCESS_CALLBACK_FILTER_ENABLED 0x1

void ApplyPatches()
{
	// Called once the exe has been unpacked. Before applying code modifications:
	// ensure that exceptions are not swallowed when dispatching certain Windows
	// messages.
	//
	// http://blog.paulbetts.org/index.php/2010/07/20/the-case-of-the-disappearing-onload-exception-user-mode-callback-exceptions-in-x64/
	// https://support.microsoft.com/en-gb/kb/976038
	BOOL(WINAPI * SetProcessUserModeExceptionPolicyPtr)(DWORD PolicyFlags);
	BOOL(WINAPI * GetProcessUserModeExceptionPolicyPtr)(LPDWORD PolicyFlags);

	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

	if (hKernel32)
	{
		*(FARPROC *)&SetProcessUserModeExceptionPolicyPtr = GetProcAddress(hKernel32, "SetProcessUserModeExceptionPolicy");
		*(FARPROC *)&GetProcessUserModeExceptionPolicyPtr = GetProcAddress(hKernel32, "GetProcessUserModeExceptionPolicy");

		if (SetProcessUserModeExceptionPolicyPtr && GetProcessUserModeExceptionPolicyPtr)
		{
			DWORD flags = 0;

			if (GetProcessUserModeExceptionPolicyPtr(&flags))
				SetProcessUserModeExceptionPolicyPtr(flags & ~PROCESS_CALLBACK_FILTER_ENABLED);
		}
	}

	DWORD oldMode = 0;
	SetErrorMode(0);
	SetThreadErrorMode(0, &oldMode);

	// Now hook everything
	PatchThreading();
	PatchWindow();
	PatchDInput();
	PatchD3D11();
	PatchAchievements();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		D3DXValidateStructures();
		EnableDumpBreakpoint();
	}

	return TRUE;
}