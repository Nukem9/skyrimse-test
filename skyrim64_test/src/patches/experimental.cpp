#include "../common.h"

#define IS_RETURN_FALSE(x) (*(BYTE *)(x + 0) == 0x32 && *(BYTE *)(x + 1) == 0xC0 && *(BYTE *)(x + 2) == 0xC3) // xor al, al; retn
#define IS_RETURN_TRUE(x)  (*(BYTE *)(x + 0) == 0xB0 && *(BYTE *)(x + 1) == 0x01 && *(BYTE *)(x + 2) == 0xC3) // mov al, 1; retn
#define IS_RETURN_0(x)     (*(BYTE *)(x + 0) == 0x33 && *(BYTE *)(x + 1) == 0xC0 && *(BYTE *)(x + 2) == 0xC3) // xor eax, eax; retn
#define IS_RETURN_LONG(x)  (*(BYTE *)(x + 0) == 0xC2 && *(BYTE *)(x + 1) == 0x00 && *(BYTE *)(x + 2) == 0x00) // retn 0
#define IS_RETURN(x)       (*(BYTE *)(x + 0) == 0xC3)                                                         // retn

void ExperimentalPatchEmptyFunctions()
{
	uintptr_t codeStart = g_CodeBase;
	uintptr_t codeEnd = codeStart + g_CodeSize;
	uint64_t patchCount = 0;

	//
	// Patch the EXE text section calls/jumps to nullsubs
	//
	// NOTE: All of these nullsubs use a long ret instruction (0xC2 0x00 0x00)
	//
	__try
	{
		for (uintptr_t i = codeStart; i < codeEnd; i++)
		{
			if (*(BYTE *)i != 0xE9 && *(BYTE *)i != 0xE8)
				continue;

			uintptr_t destination = i + *(uint32_t *)(i + 1) + 5;

			if (destination < codeStart || destination >= codeEnd)
				continue;

			if (destination % 16 != 0)
				continue;

			if (!IS_RETURN_LONG(destination))
				continue;

			// Patch the jump/call out (retn/5 byte nop)
			if (*(BYTE *)i == 0xE9)
				PatchMemory(i, (PBYTE)"\xC3\xCC\xCC\xCC\xCC", 5);
			else
				PatchMemory(i, (PBYTE)"\x0F\x1F\x44\x00\x00", 5);

			patchCount++;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

#if 0
	//
	// Disabled. The game compares certain function addresses against each other.
	//
	// Now patch all of the virtual tables. These can't be inline patched since they're function pointers.
	// Instead, remap them all to 1 common function and maybe (????) help CPU cache.
	//
	// First find 16-byte aligned functions, having only 'return false' - 'return true' - 'return 0' - 'long return' - 'return'
	//
	uintptr_t retFFunc = 0;
	uintptr_t retTFunc = 0;
	uintptr_t ret0Func = 0;
	uintptr_t retLFunc = 0;
	uintptr_t retFunc = 0;

	for (uintptr_t i = codeStart; i < codeEnd; i += 16)
	{
		if (!retFFunc && IS_RETURN_FALSE(i))
			retFFunc = i;

		if (!retTFunc && IS_RETURN_TRUE(i))
			retTFunc = i;

		if (!ret0Func && IS_RETURN_0(i))
			ret0Func = i;

		if (!retLFunc && IS_RETURN_LONG(i))
			retLFunc = i;

		if (!retFunc && IS_RETURN(i))
			retFunc = i;
	}

	// Actual patching
	uintptr_t rdataStart = g_ModuleBase + 0x15231F0;
	uintptr_t rdataEnd = g_ModuleBase + 0x1DD5000;

	auto doPatch = [&](uintptr_t Src, uintptr_t Dest)
	{
		if (IS_RETURN_FALSE(Dest))
			PatchMemory(Src, (PBYTE)&retFFunc, sizeof(uintptr_t));
		else if (IS_RETURN_TRUE(Dest))
			PatchMemory(Src, (PBYTE)&retTFunc, sizeof(uintptr_t));
		else if (IS_RETURN_0(Dest))
			PatchMemory(Src, (PBYTE)&ret0Func, sizeof(uintptr_t));
		else if (IS_RETURN_LONG(Dest))
			PatchMemory(Src, (PBYTE)&retLFunc, sizeof(uintptr_t));
		else if (IS_RETURN(Dest))
			PatchMemory(Src, (PBYTE)&retFunc, sizeof(uintptr_t));
		else
			return;

		patchCount++;
	};

	__try
	{
		for (uintptr_t i = rdataStart; i < rdataEnd; i++)
		{
			uintptr_t destination = *(uintptr_t *)i;

			if (destination < codeStart || destination >= codeEnd)
				continue;

			if (destination % 16 != 0)
				continue;

			if (i % 2 != 0)
			{
				ui::log::Add("%s: Misaligned function pointer detected (%llX => %llX).\n", __FUNCTION__, i, destination);
				continue;
			}

			doPatch(i, destination);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
#endif

	ui::log::Add("%s: %lld patches applied.\n", __FUNCTION__, patchCount);
}

void ExperimentalPatchMemInit()
{
	//
	// Remove the thousands of [code below] since they're useless checks:
	//
	// if ( dword_141ED6C88 != 2 ) // MemoryManager initialized flag
	//     sub_140C00D30((__int64)&unk_141ED6800, &dword_141ED6C88);
	//
	const char *patternStr = "\x83\x3D\x00\x00\x00\x00\x02\x74\x13\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00";
	const char *maskStr = "xx????xxxxxx????xxx????x????";

	uintptr_t codeStart = g_CodeBase;
	uintptr_t codeEnd = codeStart + g_CodeSize;
	uint64_t patchCount = 0;

	__try
	{
		for (uintptr_t i = codeStart; i < codeEnd;)
		{
			uintptr_t addr = FindPatternSimple(i, codeEnd - i, (BYTE *)patternStr, maskStr);

			if (!addr)
				break;

			PatchMemory(addr, (PBYTE)"\xEB\x1A", 2);
			i = addr + 1;
			patchCount++;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

	ui::log::Add("%s: %lld patches applied.\n", __FUNCTION__, patchCount);
}