#include "common.h"

void ApplyPatches();
BOOL WINAPI hk_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
{
	// Restore the original pointer
	DisableDumpBreakpoint();

	// Notify debuggers
	BOOL debugged;

	if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &debugged) && debugged)
		__debugbreak();

	ApplyPatches();
	return QueryPerformanceCounter(lpPerformanceCount);
}

void EnableCookieHack()
{
	// Validate DOS and NT headers
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(nullptr);
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)moduleBase;
	PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)(moduleBase + dosHeader->e_lfanew);

	Assert(dosHeader->e_magic == IMAGE_DOS_SIGNATURE);
	Assert(ntHeaders->Signature == IMAGE_NT_SIGNATURE);
	Assert(ntHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);

	// Get the load configuration section which holds the security cookie address
	auto dataDirectory	= ntHeaders->OptionalHeader.DataDirectory;
	auto sectionRVA		= dataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress;
	auto sectionSize	= dataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size;
	auto loadConfig		= (PIMAGE_LOAD_CONFIG_DIRECTORY)(moduleBase + sectionRVA);

	Assert(sectionRVA > 0 && sectionSize > 0);
	AssertMsg(loadConfig->SecurityCookie, "SecurityCookie is a null pointer!");

	// Determine the module/code section address and size
	g_ModuleBase = moduleBase;
	g_ModuleSize = ntHeaders->OptionalHeader.SizeOfImage;

	g_CodeBase = moduleBase + ntHeaders->OptionalHeader.BaseOfCode;
	g_CodeSize = ntHeaders->OptionalHeader.SizeOfCode;

	// Set the magic value which triggers an early QueryPerformanceCounter call
	*(ULONGLONG *)loadConfig->SecurityCookie = 0x2B992DDFA232;
}

char buffer[48];
ULONG_PTR proc;

void DisableDumpBreakpoint()
{
	// Restore the original QPC pointer
	PatchIAT(QueryPerformanceCounter, "kernel32.dll", "QueryPerformanceCounter");

	// Restore the original NtSetInformationThread code
	PatchMemory(proc, (PBYTE)&buffer, sizeof(buffer));
}

void EnableDumpBreakpoint()
{
	// Force QPC to be called early in the exe entrypoint (for easier dumping)
	EnableCookieHack();
	PatchIAT(hk_QueryPerformanceCounter, "kernel32.dll", "QueryPerformanceCounter");

	// Kill steam's unpacker call to NtSetInformationThread(ThreadHideFromDebugger)
	proc = (ULONG_PTR)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtSetInformationThread");

	if (proc)
	{
		memcpy(buffer, (PVOID)proc, sizeof(buffer));
		PatchMemory(proc, (PBYTE)"\xB8\x00\x00\x00\x00\xC3", 6);
	}
}