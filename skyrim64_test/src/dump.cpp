#include "common.h"

char TempNTSIT[16];
ULONG_PTR TempNTSITAddress;
LONG(NTAPI * NtSetInformationThread)(HANDLE ThreadHandle, LONG ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);

void ApplyPatches();
BOOL WINAPI hk_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
{
	// Restore the original pointer
	DumpDisableBreakpoint();

	// Notify debugger
	__try
	{
		__debugbreak();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

	ApplyPatches();
	return QueryPerformanceCounter(lpPerformanceCount);
}

LONG NTAPI hk_NtSetInformationThread(HANDLE ThreadHandle, LONG ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength)
{
	if (ThreadInformationClass == 0x11)
		return 0;

	return NtSetInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
}

void DumpEnableBreakpoint()
{
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(nullptr);
	PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)(moduleBase + ((PIMAGE_DOS_HEADER)moduleBase)->e_lfanew);

	// Get the load configuration section which holds the security cookie address
	auto dataDirectory	= ntHeaders->OptionalHeader.DataDirectory;
	auto sectionRVA		= dataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress;
	auto sectionSize	= dataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size;
	auto loadConfig		= (PIMAGE_LOAD_CONFIG_DIRECTORY)(moduleBase + sectionRVA);

	Assert(sectionRVA > 0 && sectionSize > 0);
	AssertMsg(loadConfig->SecurityCookie, "SecurityCookie is a null pointer!");

	// Determine the module/code section addresses and sizes
	g_ModuleBase = moduleBase;
	g_ModuleSize = ntHeaders->OptionalHeader.SizeOfImage;

	Assert(XUtil::GetPESectionRange(moduleBase, ".text", &g_CodeBase, &g_CodeEnd));
	Assert(XUtil::GetPESectionRange(moduleBase, ".rdata", &g_RdataBase, &g_RdataEnd));
	Assert(XUtil::GetPESectionRange(moduleBase, ".data", &g_DataBase, &g_DataEnd));

	uintptr_t tempBssStart;
	uintptr_t tempBssEnd;

	if (XUtil::GetPESectionRange(moduleBase, ".textbss", &tempBssStart, &tempBssEnd))
	{
		g_CodeBase = std::min(g_CodeBase, tempBssStart);
		g_CodeEnd = std::max(g_CodeEnd, tempBssEnd);
	}

	// Set the magic value which triggers an early QueryPerformanceCounter call
	*(uint64_t *)loadConfig->SecurityCookie = 0x2B992DDFA232;
	PatchIAT(hk_QueryPerformanceCounter, "kernel32.dll", "QueryPerformanceCounter");

	// Kill steam's unpacker call to NtSetInformationThread(ThreadHideFromDebugger)
	TempNTSITAddress = (uintptr_t)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtSetInformationThread");

	if (TempNTSITAddress)
	{
		memcpy(&TempNTSIT, (void *)TempNTSITAddress, sizeof(TempNTSIT));
		*(uintptr_t *)&NtSetInformationThread = Detours::X64::DetourFunctionClass(TempNTSITAddress, &hk_NtSetInformationThread);
	}
}

void DumpDisableBreakpoint()
{
	// Restore the original QPC pointer
	PatchIAT(QueryPerformanceCounter, "kernel32.dll", "QueryPerformanceCounter");

	if (TempNTSITAddress)
	{
		// Restore the original NtSetInformationThread code
		XUtil::PatchMemory(TempNTSITAddress, (PBYTE)&TempNTSIT, sizeof(TempNTSIT));
	}
}