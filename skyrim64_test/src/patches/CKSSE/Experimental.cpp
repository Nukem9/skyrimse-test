#include "../../common.h"
#include <zydis/include/Zydis/Zydis.h>
#include <tbb/concurrent_vector.h>
#include <execution>
#include <intrin.h>
#include <chrono>
#include "LogWindow.h"

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
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x48, 0x8B, 0x40, 0x50, 0xC3 }, 15,	// return *(__int64 *)(this + 0x50);
		{ 0x48, 0x8B, 0x41, 0x50, 0xC3 },																	// mov rax, [rcx + 0x50]; retn;
		{ 0x48, 0x8B, 0x41, 0x50, 0x90 }																	// mov rax, [rcx + 0x50]; nop;
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
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x0F, 0xB6, 0x40, 0x26, 0xC3 }, 15,	// return ZERO_EXTEND(*(__int8 *)(this + 0x26));
		{ 0x0F, 0xB6, 0x41, 0x26, 0xC3 },																	// movzx eax, [rcx + 0x26]; retn;
		{ 0x0F, 0xB6, 0x41, 0x26, 0x90 }																	// movzx eax, [rcx + 0x26]; nop;
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

uint64_t ExperimentalPatchEditAndContinue()
{
	//
	// Remove any references to the giant trampoline table generated for edit & continue
	//
	// Before: [Function call] -> [E&C stub] -> [Function]
	// After:  [Function call] -> [Function]
	//
	uint64_t patchCount = 0;
	tbb::concurrent_vector<std::pair<uintptr_t, uintptr_t>> branchTargets;
	std::vector<uintptr_t> secondaryTargets;

	// Enumerate all functions present in the x64 exception directory section
	auto ntHeaders = (PIMAGE_NT_HEADERS64)(g_ModuleBase + ((PIMAGE_DOS_HEADER)g_ModuleBase)->e_lfanew);
	const auto sectionRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
	const auto sectionSize = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;

	Assert(sectionRVA > 0 && sectionSize > 0);

	auto functionEntries = (PRUNTIME_FUNCTION)(g_ModuleBase + sectionRVA);
	uint64_t functionEntryCount = sectionSize / sizeof(RUNTIME_FUNCTION);

	// Init threadsafe instruction decoder
	ZydisDecoder decoder;
	Assert(ZYDIS_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64)));

	std::for_each(std::execution::par_unseq, &functionEntries[0], &functionEntries[functionEntryCount],
	[&branchTargets, &decoder](const RUNTIME_FUNCTION& Function)
	{
		const uintptr_t ecTableStart = OFFSET(0xFB4000, 1530);
		const uintptr_t ecTableEnd = OFFSET(0x104C50D, 1530);

		for (uint32_t offset = Function.BeginAddress; offset < Function.EndAddress;)
		{
			const uintptr_t ip = g_ModuleBase + offset;
			const uint8_t opcode = *(uint8_t *)ip;
			ZydisDecodedInstruction instruction;

			if (!ZYDIS_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (void *)ip, ZYDIS_MAX_INSTRUCTION_LENGTH, ip, &instruction)))
			{
				// Decode failed. Always increase byte offset by 1.
				offset += 1;
				continue;
			}

			offset += instruction.length;

			// Must be a call or a jump
			if (opcode != 0xE9 && opcode != 0xE8)
				continue;

			uintptr_t destination = ip + *(int32_t *)(ip + 1) + 5;

			// if (destination is within E&C table)
			if (destination >= ecTableStart && destination < ecTableEnd && *(uint8_t *)destination == 0xE9)
			{
				// Find where the trampoline actually points to, then mark it for patching
				uintptr_t real = destination + *(int32_t *)(destination + 1) + 5;

				branchTargets.emplace_back(ip, real);
			}
		}
	});

	for (auto [ip, finalDest] : branchTargets)
	{
		// The instruction byte (CALL/JMP) is never changed, but displacement is
		int32_t disp = (int32_t)(finalDest - ip) - 5;
		memcpy((void *)(ip + 1), &disp, sizeof(disp));

		patchCount++;

		if (PatchNullsub(ip, finalDest, true))
			patchCount++;
		else
			secondaryTargets.emplace_back(ip);
	}

	// Secondary pass to remove nullsubs missed or created above
	for (uintptr_t target : secondaryTargets)
	{
		uintptr_t destination = target + *(int32_t *)(target + 1) + 5;

		if (PatchNullsub(target, destination, true))
			patchCount++;
	}

	return patchCount;
}

uint64_t ExperimentalPatchMemInit()
{
	//
	// Remove the thousands of [code below] since they're useless checks:
	//
	// if ( dword_141ED6C88 != 2 ) // MemoryManager initialized flag
	//     sub_140C00D30((__int64)&unk_141ED6800, &dword_141ED6C88);
	//
	auto matches = XUtil::FindPatterns(g_CodeBase, g_CodeEnd - g_CodeBase, "83 3D ? ? ? ? 02 74 13 48 8D 15 ? ? ? ? 48 8D 0D ? ? ? ? E8");

	for (uintptr_t match : matches)
		memcpy((void *)match, "\xEB\x1A", 2);

	return matches.size();
}

uint64_t ExperimentalPatchLinkedList()
{
	//
	// Optimize a linked list HasValue<T>() hot-code-path function. Checks if the 16-byte structure
	// is 0 (list->next pointer, list->data pointer)
	//
	int cpuinfo[4];
	__cpuid(cpuinfo, 1);

	const bool hasSSE41 = ((cpuinfo[2] & (1 << 19)) != 0);
	const char *pattern = "48 89 4C 24 08 48 83 EC 18 48 8B 44 24 20 48 83 78 08 00 75 14 48 8B 44 24 20 48 83 38 00 75 09 C7 04 24 01 00 00 00 EB 07 C7 04 24 00 00 00 00 0F B6 04 24 48 83 C4 18 C3";

	auto matches = XUtil::FindPatterns(g_CodeBase, g_CodeEnd - g_CodeBase, pattern);

	for (uintptr_t match : matches)
	{
		if (hasSSE41)
			memcpy((void *)match, "\xF3\x0F\x6F\x01\x66\x0F\x38\x17\xC0\x0F\x94\xC0\xC3", 13);
		else
			memcpy((void *)match, "\x48\x83\x39\x00\x75\x0A\x48\x83\x79\x08\x00\x75\x03\xB0\x01\xC3\x32\xC0\xC3", 19);
	}

	return matches.size();
}

void ExperimentalPatchOptimizations()
{
	auto timerStart = high_resolution_clock::now();

	struct
	{
		const char *Name;
		uintptr_t Start;
		uintptr_t End;
		DWORD Protection;
	} addressRanges[] =
	{
		{ nullptr, 0, 0, 0 },		// PE header
		{ ".textbss", 0, 0, 0 },	// .textbss
		{ ".text", 0, 0, 0 },		// .text
		{ ".rdata", 0, 0, 0 },		// .rdata
		{ ".data", 0, 0, 0 },		// .data
	};

	// Mark every page as writable
	for (auto& range : addressRanges)
	{
		Assert(XUtil::GetPESectionRange(g_ModuleBase, range.Name, &range.Start, &range.End));
		Assert(VirtualProtect((void *)range.Start, range.End - range.Start, PAGE_READWRITE, &range.Protection));
	}

	uint64_t count1 = ExperimentalPatchMemInit();
	uint64_t count2 = ExperimentalPatchLinkedList();
	uint64_t count3 = ExperimentalPatchEditAndContinue();

	// Then restore the old permissions
	for (auto& range : addressRanges)
	{
		BOOL ret =
			VirtualProtect((void *)range.Start, range.End - range.Start, range.Protection, &range.Protection) &&
			FlushInstructionCache(GetCurrentProcess(), (void *)range.Start, range.End - range.Start);

		Assert(ret);
	}

	auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - timerStart).count();
	LogWindow::Log("%s: (%llu + %llu + %llu) = %llu patches applied in %llums.\n", __FUNCTION__, count1, count2, count3, (count1 + count2 + count3), duration);
}