#include "common.h"

VtableIndexUtil *VtableIndexUtil::GlobalInstance;

VtableIndexUtil *VtableIndexUtil::Instance()
{
	if (!GlobalInstance)
	{
		GlobalInstance = new VtableIndexUtil();
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

int VtableIndexUtil::ForceVtableReference()
{
	return 0;
}

void XUtil::SetThreadName(uint32_t ThreadID, const char *ThreadName)
{
	if (ThreadID == GetCurrentThreadId())
	{
#if SKYRIM64_USE_VTUNE
		__itt_thread_set_name(ThreadName);
#endif

#if SKYRIM64_USE_TRACY
		tracy::SetThreadName(GetCurrentThread(), ThreadName);
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

void XUtil::Trim(char *Buffer, char C)
{
	size_t len = strlen(Buffer);

	if (len > 0 && Buffer[len - 1] == C)
		Buffer[len - 1] = '\0';
}

void XUtil::XAssert(const char *File, int Line, const char *Format, ...)
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

uint64_t XUtil::MurmurHash64A(const void *Key, size_t Len, uint64_t Seed)
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

uintptr_t XUtil::FindPattern(uintptr_t StartAddress, uintptr_t MaxSize, const uint8_t *Bytes, const char *Mask)
{
	std::vector<std::pair<uint8_t, bool>> pattern;

	for (size_t i = 0; i < strlen(Mask); i++)
		pattern.emplace_back(Bytes[i], Mask[i] == 'x');

	const uint8_t *dataStart = (uint8_t *)StartAddress;
	const uint8_t *dataEnd = (uint8_t *)StartAddress + MaxSize + 1;

	auto ret = std::search(dataStart, dataEnd, pattern.begin(), pattern.end(),
		[](uint8_t CurrentByte, std::pair<uint8_t, bool>& Pattern)
	{
		return !Pattern.second || (CurrentByte == Pattern.first);
	});

	if (ret == dataEnd)
		return 0;

	return std::distance(dataStart, ret) + StartAddress;
}

std::vector<uintptr_t> XUtil::FindPatterns(uintptr_t StartAddress, uintptr_t MaxSize, const uint8_t *Bytes, const char *Mask)
{
	std::vector<uintptr_t> results;
	std::vector<std::pair<uint8_t, bool>> pattern;

	for (size_t i = 0; i < strlen(Mask); i++)
		pattern.emplace_back(Bytes[i], Mask[i] == 'x');

	const uint8_t *dataStart = (uint8_t *)StartAddress;
	const uint8_t *dataEnd = (uint8_t *)StartAddress + MaxSize + 1;

	for (const uint8_t *i = dataStart;;)
	{
		auto ret = std::search(i, dataEnd, pattern.begin(), pattern.end(),
			[](uint8_t CurrentByte, std::pair<uint8_t, bool>& Pattern)
		{
			return !Pattern.second || (CurrentByte == Pattern.first);
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

void XUtil::PatchMemory(uintptr_t Address, uint8_t *Data, size_t Size)
{
	DWORD d = 0;
	VirtualProtect((LPVOID)Address, Size, PAGE_EXECUTE_READWRITE, &d);

	for (uintptr_t i = Address; i < (Address + Size); i++)
		*(volatile uint8_t *)i = *Data++;

	VirtualProtect((LPVOID)Address, Size, d, &d);
	FlushInstructionCache(GetCurrentProcess(), (LPVOID)Address, Size);
}

void XUtil::PatchMemoryNop(uintptr_t Address, size_t Size)
{
	DWORD d = 0;
	VirtualProtect((LPVOID)Address, Size, PAGE_EXECUTE_READWRITE, &d);

	for (uintptr_t i = Address; i < (Address + Size); i++)
		* (volatile uint8_t*)i = 0x90;

	VirtualProtect((LPVOID)Address, Size, d, &d);
	FlushInstructionCache(GetCurrentProcess(), (LPVOID)Address, Size);
}

void XUtil::DetourJump(uintptr_t Target, uintptr_t Destination)
{
	Detours::X64::DetourFunction((uint8_t *)Target, (uint8_t *)Destination);
}

void XUtil::DetourCall(uintptr_t Target, uintptr_t Destination)
{
	Detours::X64::DetourFunction((uint8_t *)Target, (uint8_t *)Destination);
	PatchMemory(Target, (PBYTE)"\xE8", 1);
}