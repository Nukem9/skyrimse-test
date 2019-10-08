#include "../../common.h"
#include <chrono>
#include "EditorUI.h"

using namespace std::chrono;

struct NullsubPatch
{
	uint8_t Signature[32];
	uint8_t SignatureLength;
	uint8_t JumpPatch[5];
	uint8_t CallPatch[5];
} const Patches[] =
{
	// Nullsub || retn; int3; int3; int3; int3; || nop;
	{ { 0xC2, 0x00, 0x00 }, 3, { 0xC3, 0xCC, 0xCC, 0xCC, 0xCC }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 } },
	{ { 0xC3 }, 1, { 0xC3, 0xCC, 0xCC, 0xCC, 0xCC }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 } },
	{ { 0x48, 0x89, 0x4C, 0x24, 0x08, 0xC3 }, 6, { 0xC3, 0xCC, 0xCC, 0xCC, 0xCC }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 } },
	{ { 0x48, 0x89, 0x54, 0x24, 0x10, 0x48, 0x89, 0x4C, 0x24, 0x08, 0xC3 }, 11, { 0xC3, 0xCC, 0xCC, 0xCC, 0xCC }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 } },
	{ { 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x83, 0xEC, 0x28, 0x48, 0x8B, 0x4C, 0x24, 0x30, 0x0F, 0x1F, 0x44, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0xC3 }, 24, { 0xC3, 0xCC, 0xCC, 0xCC, 0xCC }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 } },

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0xC3 }, 11,							// return this;
		{ 0x48, 0x89, 0xC8, 0xC3, 0xCC },																	// mov rax, rcx; retn; int3;
		{ 0x48, 0x89, 0xC8, 0x66, 0x90 }																	// mov rax, rcx; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x48, 0x8B, 0x00, 0xC3 }, 14,			// return *(__int64 *)this;
		{ 0x48, 0x8B, 0x01, 0xC3, 0xCC },																	// mov rax, [rcx]; retn; int3;
		{ 0x48, 0x8B, 0x01, 0x66, 0x90 }																	// mov rax, [rcx]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x48, 0x8B, 0x40, 0x08, 0xC3 }, 15,	// return *(__int64 *)(this + 0x8);
		{ 0x48, 0x8B, 0x41, 0x08, 0xC3 },																	// mov rax, [rcx + 0x8]; retn;
		{ 0x48, 0x8B, 0x41, 0x08, 0x90 }																	// mov rax, [rcx + 0x8]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x00, 0xC3 }, 13,				// return *(__int32 *)this;
		{ 0x8B, 0x01, 0xC3, 0xCC, 0xCC },																	// mov eax, [rcx]; retn; int3; int3;
		{ 0x8B, 0x01, 0x0F, 0x1F, 0x00 }																	// mov eax, [rcx]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x40, 0x08, 0xC3 }, 14,			// return *(__int32 *)(this + 0x8);
		{ 0x8B, 0x41, 0x08, 0xC3, 0xCC },																	// mov eax, [rcx + 0x8]; retn; int3;
		{ 0x8B, 0x41, 0x08, 0x66, 0x90 }																	// mov eax, [rcx + 0x8]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x40, 0x14, 0xC3 }, 14,			// return *(__int32 *)(this + 0x14);
		{ 0x8B, 0x41, 0x14, 0xC3, 0xCC },																	// mov eax, [rcx + 0x14]; retn; int3;
		{ 0x8B, 0x41, 0x14, 0x66, 0x90 }																	// mov eax, [rcx + 0x14]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x0F, 0xB6, 0x40, 0x08, 0xC3 }, 15,	// return ZERO_EXTEND(*(__int8 *)(this + 0x8));
		{ 0x0F, 0xB6, 0x41, 0x08, 0xC3 },																	// movzx eax, [rcx + 0x8]; retn;
		{ 0x0F, 0xB6, 0x41, 0x08, 0x90 }																	// movzx eax, [rcx + 0x8]; nop;
	},

	{
		{ 0x89, 0x54, 0x24, 0x10, 0x48, 0x89, 0x4C, 0x24, 0x08, 0x8B, 0x44, 0x24, 0x10, 0x48, 0x8B, 0x4C, 0x24, 0x08, 0x0F, 0xB7, 0x04, 0x41, 0xC3 }, 23,	// return ZERO_EXTEND(*(unsigned __int16 *)(a1 + 2i64 * a2));
		{ 0x0F, 0xB7, 0x04, 0x51, 0xC3 },																													// movzx eax, word ptr ds:[rcx+rdx*2]; retn;
		{ 0x0F, 0xB7, 0x04, 0x51, 0x90 }																													// movzx eax, word ptr ds:[rcx+rdx*2]; nop;
	},
};

bool PatchNullsub(uintptr_t SourceAddress, uintptr_t TargetFunction, bool Extended)
{
	const bool isJump = (*(uint8_t *)SourceAddress == 0xE9);
	const void *dest = (void *)TargetFunction;

	//
	// Check if the given function is "unoptimized" and remove the branch completely. Extended
	// nullsub checking is riskier because of potential false positives.
	//
	for (uint32_t i = 0; i < ARRAYSIZE(Patches); i++)
	{
		if (!Extended && i > 0)
			break;

		if (!memcmp(dest, Patches[i].Signature, Patches[i].SignatureLength))
		{
			if (isJump)
				memcpy((void *)SourceAddress, Patches[i].JumpPatch, 5);
			else
				memcpy((void *)SourceAddress, Patches[i].CallPatch, 5);

			return true;
		}
	}

	return false;
}

uint64_t ExperimentalPatchMemInit()
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

	for (uintptr_t i = g_CodeBase; i < g_CodeEnd;)
	{
		uintptr_t addr = XUtil::FindPattern(i, g_CodeEnd - i, (uint8_t *)patternStr, maskStr);

		if (!addr)
			break;

		memcpy((void *)addr, (const void *)"\xEB\x1A", 2);
		i = addr + 1;
		patchCount++;
	}

	return patchCount;
}

uint64_t ExperimentalPatchEditAndContinue()
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
		if (*(uint8_t *)i != 0xE9 && *(uint8_t *)i != 0xE8)
			continue;

		uintptr_t destination = i + *(int32_t *)(i + 1) + 5;

		if (isWithinECTable(destination))
		{
			// Find where the trampoline actually points to, then remove it
			uintptr_t real = destination + *(int32_t *)(destination + 1) + 5;

			uint8_t data[5];
			data[0] = *(uint8_t *)i;
			*(int32_t *)&data[1] = (int32_t)(real - i) - 5;

			memcpy((void *)i, data, sizeof(data));
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
	
	return patchCount;
}

void ExperimentalPatchOptimizations()
{
	auto timerStart = high_resolution_clock::now();

	std::tuple<uintptr_t, uintptr_t, DWORD> addressRanges[] =
	{
		std::make_tuple(g_ModuleBase + 0x0, 0x1000, 0),			// PE header
		std::make_tuple(g_ModuleBase + 0x1000, 0xFB3000, 0),	// .textbss
		std::make_tuple(g_ModuleBase + 0xFB4000, 0x206D000, 0),	// .text
		std::make_tuple(g_ModuleBase + 0x3021000, 0x8CA000, 0),	// .rdata
		std::make_tuple(g_ModuleBase + 0x38EB000, 0x2129000, 0),// .data
	};

	// Mark every page as writable
	for (auto& range : addressRanges)
	{
		BOOL ret = VirtualProtect((void *)std::get<0>(range), std::get<1>(range), PAGE_READWRITE, &std::get<2>(range));

		Assert(ret);
	}

	uint64_t count1 = ExperimentalPatchEditAndContinue();
	uint64_t count2 = ExperimentalPatchMemInit();

	// Then restore the old permissions
	for (auto& range : addressRanges)
	{
		BOOL ret =
			VirtualProtect((void *)std::get<0>(range), std::get<1>(range), std::get<2>(range), &std::get<2>(range)) &&
			FlushInstructionCache(GetCurrentProcess(), (void *)std::get<0>(range), std::get<1>(range));

		Assert(ret);
	}

	auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - timerStart).count();
	EditorUI_Log("%s: (%llu + %llu) = %llu patches applied in %llums.\n", __FUNCTION__, count1, count2, (count1 + count2), duration);
}