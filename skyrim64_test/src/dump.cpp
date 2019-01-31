#include "common.h"
#include <DbgHelp.h>
#include <atomic>

char TempNTSIT[16];
ULONG_PTR TempNTSITAddress;
std::atomic_uint32_t g_DumpTargetThreadId;
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

	g_CodeBase = std::numeric_limits<uintptr_t>::max();
	g_RdataBase = std::numeric_limits<uintptr_t>::max();
	g_DataBase = std::numeric_limits<uintptr_t>::max();

	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);

	for (uint32_t i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++)
	{
		uintptr_t start = g_ModuleBase + section->VirtualAddress;
		uintptr_t end = g_ModuleBase + section->VirtualAddress + section->Misc.VirtualSize;

		// Name might not be null-terminated
		char sectionName[sizeof(IMAGE_SECTION_HEADER::Name) + 1];
		memset(sectionName, 0, sizeof(sectionName));
		memcpy(sectionName, section->Name, sizeof(IMAGE_SECTION_HEADER::Name));

		if (!_stricmp(sectionName, ".text") || !_stricmp(sectionName, ".textbss"))
		{
			if (section->Misc.VirtualSize > 0x10000)
			{
				g_CodeBase = std::min(g_CodeBase, start);
				g_CodeEnd = std::max(g_CodeEnd, end);
			}
		}
		else if (!_stricmp(sectionName, ".rdata"))
		{
			g_RdataBase = std::min(g_RdataBase, start);
			g_RdataEnd = std::max(g_RdataEnd, end);
		}
		else if (!_stricmp(sectionName, ".data"))
		{
			g_DataBase = std::min(g_DataBase, start);
			g_DataEnd = std::max(g_DataEnd, end);
		}
	}

	// Set the magic value which triggers an early QueryPerformanceCounter call
	*(ULONGLONG *)loadConfig->SecurityCookie = 0x2B992DDFA232;
	PatchIAT(hk_QueryPerformanceCounter, "kernel32.dll", "QueryPerformanceCounter");

	// Kill steam's unpacker call to NtSetInformationThread(ThreadHideFromDebugger)
	TempNTSITAddress = (uintptr_t)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtSetInformationThread");

	if (TempNTSITAddress)
	{
		memcpy(&TempNTSIT, (void *)TempNTSITAddress, sizeof(TempNTSIT));
		*(uint8_t **)&NtSetInformationThread = Detours::X64::DetourFunctionClass((PBYTE)TempNTSITAddress, &hk_NtSetInformationThread);
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

DWORD WINAPI DumpWriterThread(LPVOID Arg)
{
	Assert(Arg);

	char fileName[MAX_PATH];
	bool dumpWritten = false;

	PEXCEPTION_POINTERS exceptionInfo = (PEXCEPTION_POINTERS)Arg;
	auto miniDumpWriteDump = (decltype(&MiniDumpWriteDump))GetProcAddress(LoadLibraryA("dbghelp.dll"), "MiniDumpWriteDump");

	if (miniDumpWriteDump)
	{
		// Create a dump in the same folder of the exe itself
		char exePath[MAX_PATH];
		GetModuleFileNameA(GetModuleHandle(nullptr), exePath, ARRAYSIZE(exePath));

		SYSTEMTIME sysTime;
		GetSystemTime(&sysTime);
		sprintf_s(fileName, "%s_%4d%02d%02d_%02d%02d%02d.dmp", exePath, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

		HANDLE file = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (file != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
			dumpInfo.ThreadId = g_DumpTargetThreadId.load();
			dumpInfo.ExceptionPointers = exceptionInfo;
			dumpInfo.ClientPointers = FALSE;

			uint32_t dumpFlags = MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo;
			dumpWritten = miniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, (MINIDUMP_TYPE)dumpFlags, &dumpInfo, nullptr, nullptr) != FALSE;

			CloseHandle(file);
		}
	}
	else
	{
		strcpy_s(fileName, "UNABLE TO LOAD DBGHELP.DLL");
	}

	const char *message = nullptr;
	const char *reason = nullptr;

	if (dumpWritten)
		message = "FATAL ERROR\n\nThe Creation Kit encountered a fatal error and has crashed.\n\nReason: %s (0x%08X).\n\nA minidump has been written to '%s'.\n\nPlease note it may contain private information such as usernames.";
	else
		message = "FATAL ERROR\n\nThe Creation Kit encountered a fatal error and has crashed.\n\nReason: %s (0x%08X).\n\nA minidump could not be written to '%s'.\nPlease check that you have proper permissions.";

	switch (exceptionInfo->ExceptionRecord->ExceptionCode)
	{
	case 'PARM':
		reason = "An invalid parameter was sent to a function that considers invalid parameters fatal";
		break;

	case 'TERM':
		reason = "Program requested termination in an unusual way";
		break;

	case 'PURE':
		reason = "Pure virtual function call";
		break;

	default:
		reason = "Unspecified exception";
		break;
	}

	XUtil::XAssert("", 0, message, reason, exceptionInfo->ExceptionRecord->ExceptionCode, fileName);
	return 0;
}

LONG WINAPI DumpExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	g_DumpTargetThreadId.store(GetCurrentThreadId());
	HANDLE threadHandle = CreateThread(nullptr, 0, DumpWriterThread, ExceptionInfo, 0, nullptr);

	if (threadHandle)
	{
		WaitForSingleObject(threadHandle, INFINITE);
		CloseHandle(threadHandle);
	}

	return EXCEPTION_CONTINUE_SEARCH;
}