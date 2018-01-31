#include "common.h"

/**
* @brief Scans a given chunk of data for the given pattern and mask.
*
* @param data          The data to scan within for the given pattern.
* @param baseAddress   The base address of where the scan data is from.
* @param lpPattern     The pattern to scan for.
* @param pszMask       The mask to compare against for wildcards.
* @param offset        The offset to add to the pointer.
* @param resultUsage   The result offset to use when locating signatures that match multiple functions.
*
* @return Pointer of the pattern found, 0 otherwise.
*
* https://github.com/learn-more/findpattern-bench/blob/master/patterns/atom0s.h
* http://atom0s.com/forums/viewtopic.php?f=5&t=4
*
*/
intptr_t FindPattern(const std::vector<unsigned char>& data, intptr_t baseAddress, const unsigned char *lpPattern, const char *pszMask, intptr_t offset, intptr_t resultUsage)
{
	// Build vectored pattern
	std::vector<std::pair<unsigned char, bool>> pattern;

	for (size_t x = 0, y = strlen(pszMask); x < y; x++)
		pattern.push_back(std::make_pair(lpPattern[x], pszMask[x] == 'x'));

	auto scanStart = data.begin();
	auto resultCnt = 0;

	while (true)
	{
		// Search for the pattern
		auto ret = std::search(scanStart, data.end(), pattern.begin(), pattern.end(),
			[&](unsigned char curr, std::pair<unsigned char, bool> currPattern)
		{
			return (!currPattern.second) || curr == currPattern.first;
		});

		// Did we find a match?
		if (ret != data.end())
		{
			// If we hit the usage count, return the result
			if (resultCnt == resultUsage || resultUsage == 0)
				return (std::distance(data.begin(), ret) + baseAddress) + offset;

			// Increment the found count and scan again
			++resultCnt;
			scanStart = ++ret;
		}
		else
			break;
	}

	return 0;
}

uintptr_t FindPatternSimple(uintptr_t StartAddress, uintptr_t MaxSize, const BYTE *ByteMask, const char *Mask)
{
	auto compare = [](const BYTE *pData, const BYTE *bMask, const char *szMask)
	{
		for (; *szMask; ++szMask, ++pData, ++bMask)
		{
			if (*szMask == 'x' && *pData != *bMask)
				return false;
		}

		return *szMask == '\0';
	};

	const size_t maskLen = strlen(Mask);
	for (uintptr_t i = 0; i < MaxSize - maskLen; i++)
	{
		if (compare((BYTE *)(StartAddress + i), ByteMask, Mask))
			return StartAddress + i;
	}

	return 0;
}

void PatchMemory(ULONG_PTR Address, PBYTE Data, SIZE_T Size)
{
	DWORD d = 0;
	VirtualProtect((LPVOID)Address, Size, PAGE_EXECUTE_READWRITE, &d);

	for (SIZE_T i = Address; i < (Address + Size); i++)
		*(volatile BYTE *)i = *Data++;

	VirtualProtect((LPVOID)Address, Size, d, &d);
	FlushInstructionCache(GetCurrentProcess(), (LPVOID)Address, Size);
}

void SetThreadName(DWORD dwThreadID, const char *ThreadName)
{
	if (!ThreadName)
		return;

#if SKYRIM64_USE_VTUNE
	if (dwThreadID == GetCurrentThreadId())
		__itt_thread_set_name(ThreadName);
#endif

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = ThreadName;
	info.dwThreadID = dwThreadID;
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


void XutilAssert(const char *File, int Line, const char *Format, ...)
{
	char buffer[4096];
	char message[4096];

	va_list ap;
	va_start(ap, Format);

	_vsnprintf_s(buffer, _TRUNCATE, Format, ap);
	sprintf_s(message, "%s(%d):\n\n%s", File, Line, buffer);

	MessageBoxA(nullptr, message, "ASSERTION", MB_ICONERROR);

	if (IsDebuggerPresent())
		__debugbreak();

	ExitProcess(1);
}

vtable_index_util *vtable_index_util::GlobalInstance;

vtable_index_util *vtable_index_util::Instance()
{
	if (!GlobalInstance)
	{
		GlobalInstance = new vtable_index_util;
		GlobalInstance->ForceVtableReference();

		// Overwrite this class's vtable pointers
		static const VtableIndexFn VtableIndexArray[] =
		{
			[]() -> int { return 0; },
			[]() -> int { return 1; },
			[]() -> int { return 2; },
			[]() -> int { return 3; },
			[]() -> int { return 4; },
			[]() -> int { return 5; },
			[]() -> int { return 6; },
			[]() -> int { return 7; },
			[]() -> int { return 8; },
			[]() -> int { return 9; },
			[]() -> int { return 10; },
			[]() -> int { return 11; },
			[]() -> int { return 12; },
			[]() -> int { return 13; },
			[]() -> int { return 14; },
			[]() -> int { return 15; },
			[]() -> int { return 16; },
			[]() -> int { return 17; },
			[]() -> int { return 18; },
			[]() -> int { return 19; },
			[]() -> int { return 20; },
			[]() -> int { return 21; },
			[]() -> int { return 22; },
			[]() -> int { return 23; },
			[]() -> int { return 24; },
			[]() -> int { return 25; },
			[]() -> int { return 26; },
			[]() -> int { return 27; },
			[]() -> int { return 28; },
			[]() -> int { return 29; },
			[]() -> int { return 30; },
			[]() -> int { return 31; },
			[]() -> int { return 32; },
			[]() -> int { return 33; },
			[]() -> int { return 34; },
			[]() -> int { return 35; },
			[]() -> int { return 36; },
			[]() -> int { return 37; },
			[]() -> int { return 38; },
			[]() -> int { return 39; },
			[]() -> int { return 40; },
		};

		*(uintptr_t *)GlobalInstance = (uintptr_t)&VtableIndexArray;
	}

	return GlobalInstance;
}

int vtable_index_util::ForceVtableReference()
{
	return 0;
}