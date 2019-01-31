#include "../common.h"

#define IS_RETURN_FALSE(x) (*(BYTE *)(x + 0) == 0x32 && *(BYTE *)(x + 1) == 0xC0 && *(BYTE *)(x + 2) == 0xC3) // xor al, al; retn
#define IS_RETURN_TRUE(x)  (*(BYTE *)(x + 0) == 0xB0 && *(BYTE *)(x + 1) == 0x01 && *(BYTE *)(x + 2) == 0xC3) // mov al, 1; retn
#define IS_RETURN_0(x)     (*(BYTE *)(x + 0) == 0x33 && *(BYTE *)(x + 1) == 0xC0 && *(BYTE *)(x + 2) == 0xC3) // xor eax, eax; retn
#define IS_RETURN_LONG(x)  (*(BYTE *)(x + 0) == 0xC2 && *(BYTE *)(x + 1) == 0x00 && *(BYTE *)(x + 2) == 0x00) // retn 0
#define IS_RETURN(x)       (*(BYTE *)(x + 0) == 0xC3)                                                         // retn

bool PatchNullsub(uintptr_t SourceAddress, uintptr_t TargetFunction, bool Extended)
{
	const bool isJump = (*(BYTE *)SourceAddress == 0xE9);
	const void *dest = (void *)TargetFunction;

	//
	// Check if the given function is "unoptimized" and remove the branch completely. Extended
	// nullsub checking is riskier because of potential false positives.
	//
	const uint8_t signature1[] = { 0xC2, 0x00, 0x00 };// Real nullsub

	if (!memcmp(dest, signature1, sizeof(signature1)))
	{
		if (isJump)
			XUtil::PatchMemory(SourceAddress, (PBYTE)"\xC3\xCC\xCC\xCC\xCC", 5);// retn; int3; int3; int3; int3;
		else
			XUtil::PatchMemory(SourceAddress, (PBYTE)"\x0F\x1F\x44\x00\x00", 5);// nop;

		return true;
	}
	else if (Extended)
	{
		const uint8_t signature2[] = { 0xC3 };// Real nullsub
		const uint8_t signature3[] = { 0x48, 0x89, 0x4C, 0x24, 0x08, 0xC3 };// Effectively a nullsub
		const uint8_t signature4[] = { 0x48, 0x89, 0x54, 0x24, 0x10, 0x48, 0x89, 0x4C, 0x24, 0x08, 0xC3 };// Effectively a nullsub
		const uint8_t signature5[] = { 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x83, 0xEC, 0x28, 0x48, 0x8B, 0x4C, 0x24, 0x30, 0x0F, 0x1F, 0x44, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0xC3 };// Effectively a nullsub
		const uint8_t signature6[] = { 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x48, 0x8B, 0x00, 0xC3 };// return *(QWORD *)this;
		const uint8_t signature7[] = { 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0xC3 };// return this;
		const uint8_t signature8[] = { 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x00, 0xC3 };// return *(DWORD *)this;

		if (!memcmp(dest, signature2, sizeof(signature2)) ||
			!memcmp(dest, signature3, sizeof(signature3)) ||
			!memcmp(dest, signature4, sizeof(signature4)) ||
			!memcmp(dest, signature5, sizeof(signature5)))
		{
			if (isJump)
				XUtil::PatchMemory(SourceAddress, (PBYTE)"\xC3\xCC\xCC\xCC\xCC", 5);// retn; int3; int3; int3; int3;
			else
				XUtil::PatchMemory(SourceAddress, (PBYTE)"\x0F\x1F\x44\x00\x00", 5);// nop;

			return true;
		}
		else if (!memcmp(dest, signature6, sizeof(signature6)))
		{
			if (isJump)
				XUtil::PatchMemory(SourceAddress, (PBYTE)"\x48\x8B\x01\xC3\xCC", 5);// mov rax, [rcx]; retn; int3;
			else
				XUtil::PatchMemory(SourceAddress, (PBYTE)"\x48\x8B\x01\x66\x90", 5);// mov rax, [rcx]; nop;

			return true;
		}
		else if (!memcmp(dest, signature7, sizeof(signature7)))
		{
			if (isJump)
				XUtil::PatchMemory(SourceAddress, (PBYTE)"\x48\x89\xC8\xC3\xCC", 5);// mov rax, rcx; retn; int3;
			else
				XUtil::PatchMemory(SourceAddress, (PBYTE)"\x48\x89\xC8\x66\x90", 5);// mov rax, rcx; nop;

			return true;
		}
		else if (!memcmp(dest, signature8, sizeof(signature8)))
		{
			if (isJump)
				XUtil::PatchMemory(SourceAddress, (PBYTE)"\x8B\x01\xC3\xCC\xCC", 5);// mov eax, [rcx]; retn; int3; int3;
			else
				XUtil::PatchMemory(SourceAddress, (PBYTE)"\x8B\x01\x0F\x1F\x00", 5);// mov eax, [rcx]; nop;

			return true;
		}
	}

	return false;
}

void ExperimentalPatchEmptyFunctions()
{
	//
	// Patch the EXE text section calls/jumps to nullsubs
	//
	// NOTE: All of these nullsubs use a long ret instruction (0xC2 0x00 0x00)
	//
	uint64_t patchCount = 0;

	__try
	{
		for (uintptr_t i = g_CodeBase; i < g_CodeEnd; i++)
		{
			if (*(BYTE *)i != 0xE9 && *(BYTE *)i != 0xE8)
				continue;

			uintptr_t destination = i + *(uint32_t *)(i + 1) + 5;

			if (destination < g_CodeBase || destination >= g_CodeEnd)
				continue;

			if (destination % 16 != 0)
				continue;

			if (PatchNullsub(i, destination, false))
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
			XUtil::PatchMemory(Src, (PBYTE)&retFFunc, sizeof(uintptr_t));
		else if (IS_RETURN_TRUE(Dest))
			XUtil::PatchMemory(Src, (PBYTE)&retTFunc, sizeof(uintptr_t));
		else if (IS_RETURN_0(Dest))
			XUtil::PatchMemory(Src, (PBYTE)&ret0Func, sizeof(uintptr_t));
		else if (IS_RETURN_LONG(Dest))
			XUtil::PatchMemory(Src, (PBYTE)&retLFunc, sizeof(uintptr_t));
		else if (IS_RETURN(Dest))
			XUtil::PatchMemory(Src, (PBYTE)&retFunc, sizeof(uintptr_t));
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

	uint64_t patchCount = 0;

	__try
	{
		for (uintptr_t i = g_CodeBase; i < g_CodeEnd;)
		{
			uintptr_t addr = XUtil::FindPattern(i, g_CodeEnd - i, (BYTE *)patternStr, maskStr);

			if (!addr)
				break;

			XUtil::PatchMemory(addr, (PBYTE)"\xEB\x1A", 2);
			i = addr + 1;
			patchCount++;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

	ui::log::Add("%s: %lld patches applied.\n", __FUNCTION__, patchCount);
}

void ExperimentalPatchEditAndContinue()
{
	//
	// Remove any references to the giant trampoline table generated for edit & continue
	//
	// Before: [Function call] -> [E&C stub] -> [Function]
	// After:  [Function call] -> [Function]
	//
	uint64_t patchCount = 0;
	std::vector<uintptr_t> branchTargets;

	auto isWithinECTable = [](uintptr_t Address)
	{
		uintptr_t start = g_ModuleBase + 0xFB4000;
		uintptr_t end = g_ModuleBase + 0x104C50D;

		// Must be a jump
		if (Address >= start && Address < end && *(uint8_t *)Address == 0xE9)
			return true;

		return false;
	};

	for (uintptr_t i = g_CodeBase; i < g_CodeEnd; i++)
	{
		// Must be a call or a jump
		if (*(BYTE *)i != 0xE9 && *(BYTE *)i != 0xE8)
			continue;

		uintptr_t destination = i + *(int32_t *)(i + 1) + 5;

		if (isWithinECTable(destination))
		{
			// Find where the trampoline actually points to, then remove it
			uintptr_t real = destination + *(int32_t *)(destination + 1) + 5;

			BYTE data[5];
			data[0] = *(BYTE *)i;
			*(int32_t *)&data[1] = (int32_t)(real - i) - 5;

			XUtil::PatchMemory(i, data, sizeof(data));
			patchCount++;

			if (PatchNullsub(i, real, true))
				patchCount++;
			else
				branchTargets.push_back(i);
		}
	}

	// Secondary pass to remove nullsubs missed or created above
	for (uintptr_t target : branchTargets)
	{
		uintptr_t destination = target + *(int32_t *)(target + 1) + 5;

		if (PatchNullsub(target, destination, true))
			patchCount++;
	}

	ui::log::Add("%s: %lld patches applied.\n", __FUNCTION__, patchCount);
}