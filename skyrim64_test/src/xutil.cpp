#include "stdafx.h"
#include <ittnotify.h>
#pragma comment(lib, "libittnotify.lib")

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

#ifdef ITT_PLATFORM
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