#include "common.h"

void ApplyPatches();
BOOL WINAPI hk_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
{
	// Restore the original pointer
	DisableDumpBreakpoint();

	// Notify debuggers
	__try
	{
		BOOL debugged;

		if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &debugged) && debugged)
			__debugbreak();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

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

	// Determine the module/code section addresses and sizes
	g_ModuleBase = moduleBase;
	g_ModuleSize = ntHeaders->OptionalHeader.SizeOfImage;

	g_CodeBase = std::numeric_limits<uintptr_t>::max();
	g_RdataBase = std::numeric_limits<uintptr_t>::max();
	g_DataBase = std::numeric_limits<uintptr_t>::max();

	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);

	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++)
	{
		// Name might not be null-terminated
		char sectionName[sizeof(IMAGE_SECTION_HEADER::Name) + 1];
		memset(sectionName, 0, sizeof(sectionName));
		memcpy(sectionName, section->Name, sizeof(IMAGE_SECTION_HEADER::Name));

		uintptr_t end = g_ModuleBase + section->VirtualAddress + section->Misc.VirtualSize;

		if (!_stricmp(sectionName, ".text") || !_stricmp(sectionName, ".textbss"))
		{
			if (section->Misc.VirtualSize > 0x10000)
			{
				g_CodeBase = std::min(g_CodeBase, g_ModuleBase + section->VirtualAddress);
				g_CodeEnd = std::max(g_CodeEnd, g_ModuleBase + section->VirtualAddress + section->Misc.VirtualSize);
			}
		}
		else if (!_stricmp(sectionName, ".rdata"))
		{
			g_RdataBase = std::min(g_RdataBase, g_ModuleBase + section->VirtualAddress);
			g_RdataEnd = std::max(g_RdataEnd, g_ModuleBase + section->VirtualAddress + section->Misc.VirtualSize);
		}
		else if (!_stricmp(sectionName, ".data"))
		{
			g_DataBase = std::min(g_DataBase, g_ModuleBase + section->VirtualAddress);
			g_DataEnd = std::max(g_DataEnd, g_ModuleBase + section->VirtualAddress + section->Misc.VirtualSize);
		}
	}

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