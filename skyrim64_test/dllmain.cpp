#include "stdafx.h"

ULONG_PTR g_ModuleBase;
ULONG_PTR g_ModuleSize;

ULONG_PTR g_CodeBase;
ULONG_PTR g_CodeSize;

void Test()
{
	// Validate DOS Header
	ULONG_PTR moduleBase		= (ULONG_PTR)GetModuleHandleA(nullptr);
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)moduleBase;

	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		__debugbreak();

	// Validate PE Header and 64-bit module type
	PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)(moduleBase + dosHeader->e_lfanew);

	if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
		__debugbreak();

	if (ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		__debugbreak();

	// Get the load configuration section which holds the security cookie address
	auto dataDirectory	= ntHeaders->OptionalHeader.DataDirectory;
	DWORD sectionRVA	= dataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress;
	DWORD sectionSize	= dataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size;

	if (sectionRVA == 0 || sectionSize == 0)
		__debugbreak();

	auto loadConfig = (PIMAGE_LOAD_CONFIG_DIRECTORY)(moduleBase + sectionRVA);
	
	if (loadConfig->SecurityCookie)
	{
		// Set the cookie to the default which triggers a QueryPerformanceCounter call later on
		*(ULONGLONG *)loadConfig->SecurityCookie = 0x2B992DDFA232;
	}
	else
	{
		MessageBoxA(nullptr, "Cookie was a null pointer", "Unexpected", 0);
		__debugbreak();
	}

	// Determine the module/code section address and size
	g_ModuleBase = moduleBase;
	g_ModuleSize = ntHeaders->OptionalHeader.SizeOfImage;

	g_CodeBase = moduleBase + ntHeaders->OptionalHeader.BaseOfCode;
	g_CodeSize = ntHeaders->OptionalHeader.SizeOfCode;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		D3DXValidateStructures();
		EnableDumpBreakpoint();
		Test();

		PatchThreading();
		PatchWindow();
		PatchDInput();
		PatchD3D11();
	}

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}