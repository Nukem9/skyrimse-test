#include "common.h"
#include <atomic>
#include <thread>
#include <DbgHelp.h>

namespace XUtil
{
	HANDLE g_CrashDumpTriggerEvent;
	std::atomic<PEXCEPTION_POINTERS> g_CrashDumpExceptionInfo;
	std::atomic_uint32_t g_CrashDumpTargetThreadId;

	VtableIndexer *VtableIndexer::Instance()
	{
		if (!GlobalInstance)
		{
			GlobalInstance = new VtableIndexer();
			GlobalInstance->ForceVtableReference();

			// Overwrite this class's vtable pointers
			static const VtableIndexFn VtableIndexArray[] =
			{
#define MAKE_ENTRY(Index) []() -> int { return Index; }
				MAKE_ENTRY(0), MAKE_ENTRY(1), MAKE_ENTRY(2),
				MAKE_ENTRY(3), MAKE_ENTRY(4), MAKE_ENTRY(5),
				MAKE_ENTRY(6), MAKE_ENTRY(7), MAKE_ENTRY(8),
				MAKE_ENTRY(9), MAKE_ENTRY(10), MAKE_ENTRY(11),
				MAKE_ENTRY(12), MAKE_ENTRY(13), MAKE_ENTRY(14),
				MAKE_ENTRY(15), MAKE_ENTRY(16), MAKE_ENTRY(17),
				MAKE_ENTRY(18), MAKE_ENTRY(19), MAKE_ENTRY(20),
				MAKE_ENTRY(21), MAKE_ENTRY(22), MAKE_ENTRY(23),
				MAKE_ENTRY(24), MAKE_ENTRY(25), MAKE_ENTRY(26),
				MAKE_ENTRY(27), MAKE_ENTRY(28), MAKE_ENTRY(29),
				MAKE_ENTRY(30), MAKE_ENTRY(31), MAKE_ENTRY(32),
				MAKE_ENTRY(33), MAKE_ENTRY(34), MAKE_ENTRY(35),
				MAKE_ENTRY(36), MAKE_ENTRY(37), MAKE_ENTRY(38),
				MAKE_ENTRY(39), MAKE_ENTRY(40), MAKE_ENTRY(41),
				MAKE_ENTRY(42), MAKE_ENTRY(43), MAKE_ENTRY(44),
				MAKE_ENTRY(45), MAKE_ENTRY(46), MAKE_ENTRY(47),
				MAKE_ENTRY(48), MAKE_ENTRY(49), MAKE_ENTRY(50),
				MAKE_ENTRY(51), MAKE_ENTRY(52), MAKE_ENTRY(53),
				MAKE_ENTRY(54), MAKE_ENTRY(55), MAKE_ENTRY(56),
				MAKE_ENTRY(57), MAKE_ENTRY(58), MAKE_ENTRY(59),
				MAKE_ENTRY(60), MAKE_ENTRY(61), MAKE_ENTRY(62),
				MAKE_ENTRY(63), MAKE_ENTRY(64), MAKE_ENTRY(65),
				MAKE_ENTRY(66), MAKE_ENTRY(67), MAKE_ENTRY(68),
				MAKE_ENTRY(69), MAKE_ENTRY(70), MAKE_ENTRY(71),
				MAKE_ENTRY(72), MAKE_ENTRY(73), MAKE_ENTRY(74),
				MAKE_ENTRY(75), MAKE_ENTRY(76), MAKE_ENTRY(77),
				MAKE_ENTRY(78), MAKE_ENTRY(79), MAKE_ENTRY(80),
				MAKE_ENTRY(81), MAKE_ENTRY(82), MAKE_ENTRY(83),
				MAKE_ENTRY(84), MAKE_ENTRY(85), MAKE_ENTRY(86),
				MAKE_ENTRY(87), MAKE_ENTRY(88), MAKE_ENTRY(89),
				MAKE_ENTRY(90), MAKE_ENTRY(91), MAKE_ENTRY(92),
				MAKE_ENTRY(93), MAKE_ENTRY(94), MAKE_ENTRY(95),
				MAKE_ENTRY(96), MAKE_ENTRY(97), MAKE_ENTRY(98),
				MAKE_ENTRY(99), MAKE_ENTRY(100), MAKE_ENTRY(101),
				MAKE_ENTRY(102), MAKE_ENTRY(103), MAKE_ENTRY(104),
				MAKE_ENTRY(105), MAKE_ENTRY(106), MAKE_ENTRY(107),
				MAKE_ENTRY(108), MAKE_ENTRY(109), MAKE_ENTRY(110),
				MAKE_ENTRY(111), MAKE_ENTRY(112), MAKE_ENTRY(113),
				MAKE_ENTRY(114), MAKE_ENTRY(115), MAKE_ENTRY(116),
				MAKE_ENTRY(117), MAKE_ENTRY(118), MAKE_ENTRY(119),
				MAKE_ENTRY(120),
#undef MAKE_ENTRY
			};

			*(uintptr_t *)GlobalInstance = (uintptr_t)&VtableIndexArray;
		}

		return GlobalInstance;
	}

	int VtableIndexer::ForceVtableReference()
	{
		return 0;
	}

	void SetThreadName(uint32_t ThreadID, const char *ThreadName)
	{
		if (ThreadID == GetCurrentThreadId())
		{
#if SKYRIM64_USE_VTUNE
			__itt_thread_set_name(ThreadName);
#endif

#if SKYRIM64_USE_TRACY
			tracy::SetThreadName(ThreadName);
#endif
		}

#pragma pack(push, 8)  
		const DWORD MS_VC_EXCEPTION = 0x406D1388;

		typedef struct tagTHREADNAME_INFO
		{
			DWORD dwType;		// Must be 0x1000.  
			LPCSTR szName;		// Pointer to name (in user addr space).  
			DWORD dwThreadID;	// Thread ID (-1=caller thread).  
			DWORD dwFlags;		// Reserved for future use, must be zero.  
		} THREADNAME_INFO;
#pragma pack(pop)

		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = ThreadName;
		info.dwThreadID = ThreadID;
		info.dwFlags = 0;

		__try
		{
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

	void Trim(char *Buffer, char C)
	{
		size_t len = strlen(Buffer);

		if (len > 0 && Buffer[len - 1] == C)
			Buffer[len - 1] = '\0';
	}

	void XAssert(const char *File, int Line, const char *Format, ...)
	{
		char buffer[4096];
		char message[4096];

		va_list ap;
		va_start(ap, Format);

		_vsnprintf_s(buffer, _TRUNCATE, Format, ap);
		sprintf_s(message, "%s(%d):\n\n%s", File, Line, buffer);

		MessageBoxA(nullptr, message, "ASSERTION", MB_ICONERROR);

		if (IsDebuggerPresent())
		{
			OutputDebugStringA(message);
			__debugbreak();
		}

		TerminateProcess(GetCurrentProcess(), 1);
		__assume(0);
	}

	uint64_t MurmurHash64A(const void *Key, size_t Len, uint64_t Seed)
	{
		/*-----------------------------------------------------------------------------
		// https://github.com/abrandoned/murmur2/blob/master/MurmurHash2.c#L65
		// MurmurHash2, 64-bit versions, by Austin Appleby
		//
		// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
		// and endian-ness issues if used across multiple platforms.
		//
		// 64-bit hash for 64-bit platforms
		*/
		const uint64_t m = 0xc6a4a7935bd1e995ull;
		const int r = 47;

		uint64_t h = Seed ^ (Len * m);

		const uint64_t *data = (const uint64_t *)Key;
		const uint64_t *end = data + (Len / 8);

		while (data != end)
		{
			uint64_t k = *data++;

			k *= m;
			k ^= k >> r;
			k *= m;

			h ^= k;
			h *= m;
		}

		const unsigned char *data2 = (const unsigned char *)data;

		switch (Len & 7)
		{
		case 7: h ^= ((uint64_t)data2[6]) << 48;
		case 6: h ^= ((uint64_t)data2[5]) << 40;
		case 5: h ^= ((uint64_t)data2[4]) << 32;
		case 4: h ^= ((uint64_t)data2[3]) << 24;
		case 3: h ^= ((uint64_t)data2[2]) << 16;
		case 2: h ^= ((uint64_t)data2[1]) << 8;
		case 1: h ^= ((uint64_t)data2[0]);
			h *= m;
		}

		h ^= h >> r;
		h *= m;
		h ^= h >> r;

		return h;
	}

	uintptr_t FindPattern(uintptr_t StartAddress, uintptr_t MaxSize, const char *Mask)
	{
		std::vector<std::pair<uint8_t, bool>> pattern;

		for (size_t i = 0; i < strlen(Mask);)
		{
			if (Mask[i] != '?')
			{
				pattern.emplace_back((uint8_t)strtoul(&Mask[i], nullptr, 16), false);
				i += 3;
			}
			else
			{
				pattern.emplace_back(0x00, true);
				i += 2;
			}
		}

		const uint8_t *dataStart = (uint8_t *)StartAddress;
		const uint8_t *dataEnd = (uint8_t *)StartAddress + MaxSize + 1;

		auto ret = std::search(dataStart, dataEnd, pattern.begin(), pattern.end(),
			[](uint8_t CurrentByte, std::pair<uint8_t, bool>& Pattern)
		{
			return Pattern.second || (CurrentByte == Pattern.first);
		});

		if (ret == dataEnd)
			return 0;

		return std::distance(dataStart, ret) + StartAddress;
	}

	std::vector<uintptr_t> FindPatterns(uintptr_t StartAddress, uintptr_t MaxSize, const char *Mask)
	{
		std::vector<uintptr_t> results;
		std::vector<std::pair<uint8_t, bool>> pattern;

		for (size_t i = 0; i < strlen(Mask);)
		{
			if (Mask[i] != '?')
			{
				pattern.emplace_back((uint8_t)strtoul(&Mask[i], nullptr, 16), false);
				i += 3;
			}
			else
			{
				pattern.emplace_back(0x00, true);
				i += 2;
			}
		}

		const uint8_t *dataStart = (uint8_t *)StartAddress;
		const uint8_t *dataEnd = (uint8_t *)StartAddress + MaxSize + 1;

		for (const uint8_t *i = dataStart;;)
		{
			auto ret = std::search(i, dataEnd, pattern.begin(), pattern.end(),
				[](uint8_t CurrentByte, std::pair<uint8_t, bool>& Pattern)
			{
				return Pattern.second || (CurrentByte == Pattern.first);
			});

			// No byte pattern matched, exit loop
			if (ret == dataEnd)
				break;

			uintptr_t addr = std::distance(dataStart, ret) + StartAddress;
			results.push_back(addr);

			i = (uint8_t *)(addr + 1);
		}

		return results;
	}

	bool GetPESectionRange(uintptr_t ModuleBase, const char *Section, uintptr_t *Start, uintptr_t *End)
	{
		auto ntHeaders = (PIMAGE_NT_HEADERS64)(ModuleBase + ((PIMAGE_DOS_HEADER)ModuleBase)->e_lfanew);
		auto section = IMAGE_FIRST_SECTION(ntHeaders);

		// Assume PE header if no section
		if (!Section || strlen(Section) <= 0)
		{
			if (Start)
				*Start = ModuleBase;

			if (End)
				*End = ModuleBase + ntHeaders->OptionalHeader.SizeOfHeaders;

			return true;
		}

		for (uint32_t i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++)
		{
			// Name might not be null-terminated
			char sectionName[sizeof(IMAGE_SECTION_HEADER::Name) + 1] = {};
			memcpy(sectionName, section->Name, sizeof(IMAGE_SECTION_HEADER::Name));

			if (!strcmp(sectionName, Section))
			{
				if (Start)
					*Start = ModuleBase + section->VirtualAddress;

				if (End)
					*End = ModuleBase + section->VirtualAddress + section->Misc.VirtualSize;

				return true;
			}
		}

		return false;
	}

	void PatchMemory(uintptr_t Address, const uint8_t *Data, size_t Size)
	{
		DWORD d = 0;
		VirtualProtect((LPVOID)Address, Size, PAGE_EXECUTE_READWRITE, &d);

		for (uintptr_t i = Address; i < (Address + Size); i++)
			*(volatile uint8_t *)i = *Data++;

		VirtualProtect((LPVOID)Address, Size, d, &d);
		FlushInstructionCache(GetCurrentProcess(), (LPVOID)Address, Size);
	}

	void PatchMemory(uintptr_t Address, std::initializer_list<uint8_t> Data)
	{
		PatchMemory(Address, Data.begin(), Data.size());
	}

	void PatchMemoryNop(uintptr_t Address, size_t Size)
	{
		DWORD d = 0;
		VirtualProtect((LPVOID)Address, Size, PAGE_EXECUTE_READWRITE, &d);

		for (uintptr_t i = Address; i < (Address + Size); i++)
			*(volatile uint8_t *)i = 0x90;

		VirtualProtect((LPVOID)Address, Size, d, &d);
		FlushInstructionCache(GetCurrentProcess(), (LPVOID)Address, Size);
	}

	void DetourJump(uintptr_t Target, uintptr_t Destination)
	{
		Detours::X64::DetourFunction(Target, Destination, Detours::X64Option::USE_REL32_JUMP);
	}

	void DetourCall(uintptr_t Target, uintptr_t Destination)
	{
		Detours::X64::DetourFunction(Target, Destination, Detours::X64Option::USE_REL32_CALL);
	}

	void InstallCrashDumpHandler()
	{
		g_CrashDumpTriggerEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		SetUnhandledExceptionFilter(CrashDumpExceptionHandler);

		std::thread t([]()
		{
			if (WaitForSingleObject(g_CrashDumpTriggerEvent, INFINITE) != WAIT_OBJECT_0)
				return;

			char fileName[MAX_PATH];
			bool dumpWritten = false;

			auto ehInfo = g_CrashDumpExceptionInfo.load();
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
					MINIDUMP_EXCEPTION_INFORMATION dumpInfo
					{
						.ThreadId = g_CrashDumpTargetThreadId,
						.ExceptionPointers = ehInfo,
						.ClientPointers = FALSE,
					};

					auto dumpFlags = (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo);
					dumpWritten = miniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, dumpFlags, &dumpInfo, nullptr, nullptr) != FALSE;

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

			switch (ehInfo->ExceptionRecord->ExceptionCode)
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

			XUtil::XAssert("", 0, message, reason, ehInfo->ExceptionRecord->ExceptionCode, fileName);
		});

		t.detach();
	}

	LONG WINAPI CrashDumpExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo)
	{
		g_CrashDumpExceptionInfo = ExceptionInfo;
		g_CrashDumpTargetThreadId = GetCurrentThreadId();

		SetEvent(g_CrashDumpTriggerEvent);
		Sleep(INFINITE);

		return EXCEPTION_CONTINUE_SEARCH;
	}
}