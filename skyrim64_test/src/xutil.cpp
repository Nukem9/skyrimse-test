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
	{
		OutputDebugStringA(message);
		__debugbreak();
	}

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

int vtable_index_util::ForceVtableReference()
{
	return 0;
}