#include "../../common.h"
#include <zydis/include/Zydis/Zydis.h>
#include <tbb/concurrent_vector.h>
#include <execution>
#include <intrin.h>
#include <chrono>
#include "Editor.h"
#include "LogWindow.h"

using namespace std::chrono;

struct NullsubPatch
{
	std::initializer_list<uint8_t> Signature;
	uint8_t JumpPatch[5];
	uint8_t CallPatch[5];
} const Patches[] =
{
	// Nullsub || retn; int3; int3; int3; int3; || nop;
	{ { 0xC2, 0x00, 0x00 }, { 0xC3, 0xCC, 0xCC, 0xCC, 0xCC }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 } },
	{ { 0xC3 }, { 0xC3, 0xCC, 0xCC, 0xCC, 0xCC }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 } },
	{ { 0x48, 0x89, 0x4C, 0x24, 0x08, 0xC3 }, { 0xC3, 0xCC, 0xCC, 0xCC, 0xCC }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 } },
	{ { 0x48, 0x89, 0x54, 0x24, 0x10, 0x48, 0x89, 0x4C, 0x24, 0x08, 0xC3 }, { 0xC3, 0xCC, 0xCC, 0xCC, 0xCC }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 } },
	{ { 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x83, 0xEC, 0x28, 0x48, 0x8B, 0x4C, 0x24, 0x30, 0x0F, 0x1F, 0x44, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0xC3 }, { 0xC3, 0xCC, 0xCC, 0xCC, 0xCC }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 } },

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0xC3 },								// return this;
		{ 0x48, 0x89, 0xC8, 0xC3, 0xCC },																	// mov rax, rcx; retn; int3;
		{ 0x48, 0x89, 0xC8, 0x66, 0x90 }																	// mov rax, rcx; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x48, 0x8B, 0x00, 0xC3 },				// return *(__int64 *)this;
		{ 0x48, 0x8B, 0x01, 0xC3, 0xCC },																	// mov rax, [rcx]; retn; int3;
		{ 0x48, 0x8B, 0x01, 0x66, 0x90 }																	// mov rax, [rcx]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x48, 0x8B, 0x40, 0x08, 0xC3 },		// return *(__int64 *)(this + 0x8);
		{ 0x48, 0x8B, 0x41, 0x08, 0xC3 },																	// mov rax, [rcx + 0x8]; retn;
		{ 0x48, 0x8B, 0x41, 0x08, 0x90 }																	// mov rax, [rcx + 0x8]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x48, 0x8B, 0x40, 0x50, 0xC3 },		// return *(__int64 *)(this + 0x50);
		{ 0x48, 0x8B, 0x41, 0x50, 0xC3 },																	// mov rax, [rcx + 0x50]; retn;
		{ 0x48, 0x8B, 0x41, 0x50, 0x90 }																	// mov rax, [rcx + 0x50]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x00, 0xC3 },					// return *(__int32 *)this;
		{ 0x8B, 0x01, 0xC3, 0xCC, 0xCC },																	// mov eax, [rcx]; retn; int3; int3;
		{ 0x8B, 0x01, 0x0F, 0x1F, 0x00 }																	// mov eax, [rcx]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x40, 0x08, 0xC3 },				// return *(__int32 *)(this + 0x8);
		{ 0x8B, 0x41, 0x08, 0xC3, 0xCC },																	// mov eax, [rcx + 0x8]; retn; int3;
		{ 0x8B, 0x41, 0x08, 0x66, 0x90 }																	// mov eax, [rcx + 0x8]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x40, 0x14, 0xC3 },				// return *(__int32 *)(this + 0x14);
		{ 0x8B, 0x41, 0x14, 0xC3, 0xCC },																	// mov eax, [rcx + 0x14]; retn; int3;
		{ 0x8B, 0x41, 0x14, 0x66, 0x90 }																	// mov eax, [rcx + 0x14]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x0F, 0xB6, 0x40, 0x08, 0xC3 },		// return ZERO_EXTEND(*(__int8 *)(this + 0x8));
		{ 0x0F, 0xB6, 0x41, 0x08, 0xC3 },																	// movzx eax, [rcx + 0x8]; retn;
		{ 0x0F, 0xB6, 0x41, 0x08, 0x90 }																	// movzx eax, [rcx + 0x8]; nop;
	},

	{
		{ 0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x8B, 0x44, 0x24, 0x08, 0x0F, 0xB6, 0x40, 0x26, 0xC3 },		// return ZERO_EXTEND(*(__int8 *)(this + 0x26));
		{ 0x0F, 0xB6, 0x41, 0x26, 0xC3 },																	// movzx eax, [rcx + 0x26]; retn;
		{ 0x0F, 0xB6, 0x41, 0x26, 0x90 }																	// movzx eax, [rcx + 0x26]; nop;
	},

	{
		{ 0x89, 0x54, 0x24, 0x10, 0x48, 0x89, 0x4C, 0x24, 0x08, 0x8B, 0x44, 0x24, 0x10, 0x48, 0x8B, 0x4C, 0x24, 0x08, 0x0F, 0xB7, 0x04, 0x41, 0xC3 },	// return ZERO_EXTEND(*(unsigned __int16 *)(a1 + 2i64 * a2));
		{ 0x0F, 0xB7, 0x04, 0x51, 0xC3 },																												// movzx eax, word ptr ds:[rcx+rdx*2]; retn;
		{ 0x0F, 0xB7, 0x04, 0x51, 0x90 }																												// movzx eax, word ptr ds:[rcx+rdx*2]; nop;
	},
};

const NullsubPatch *FindNullsubPatch(uintptr_t SourceAddress, uintptr_t TargetFunction)
{
	for (auto& patch : Patches)
	{
		if (!memcmp((void *)TargetFunction, patch.Signature.begin(), patch.Signature.size()))
			return &patch;
	}

	return nullptr;
}

bool PatchNullsub(uintptr_t SourceAddress, uintptr_t TargetFunction, const NullsubPatch *Patch = nullptr)
{
	const bool isJump = *(uint8_t *)SourceAddress == 0xE9;

	// Check if the given function is "unoptimized" and remove the branch completely
	if (!Patch)
		Patch = FindNullsubPatch(SourceAddress, TargetFunction);

	if (Patch)
	{
		if (isJump)
			memcpy((void *)SourceAddress, Patch->JumpPatch, 5);
		else
			memcpy((void *)SourceAddress, Patch->CallPatch, 5);

		return true;
	}

	return false;
}

uint64_t ExperimentalPatchEditAndContinue()
{
	//
	// Remove any references to the giant trampoline table generated for edit & continue
	//
	// Before: [Function call] -> [E&C trampoline] -> [Function]
	// After:  [Function call] -> [Function]
	//
	tbb::concurrent_vector<std::pair<uintptr_t, const NullsubPatch *>> nullsubTargets;
	tbb::concurrent_vector<uintptr_t> branchTargets;

	// Enumerate all functions present in the x64 exception directory section
	auto ntHeaders = (PIMAGE_NT_HEADERS64)(g_ModuleBase + ((PIMAGE_DOS_HEADER)g_ModuleBase)->e_lfanew);
	const auto sectionRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
	const auto sectionSize = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;

	Assert(sectionRVA > 0 && sectionSize > 0);

	auto functionEntries = (PRUNTIME_FUNCTION)(g_ModuleBase + sectionRVA);
	auto functionEntryCount = sectionSize / sizeof(RUNTIME_FUNCTION);

	// Init threadsafe instruction decoder
	ZydisDecoder decoder;
	Assert(ZYDIS_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64)));

	std::for_each(std::execution::par_unseq, &functionEntries[0], &functionEntries[functionEntryCount],
	[&branchTargets, &nullsubTargets, &decoder](const RUNTIME_FUNCTION& Function)
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
				// Determine where the E&C trampoline jumps to, then remove it. Each function is processed separately so thread
				// safety is not an issue when patching. The 0xE9 opcode never changes.
				uintptr_t real = destination + *(int32_t *)(destination + 1) + 5;

				int32_t disp = (int32_t)(real - ip) - 5;
				memcpy((void *)(ip + 1), &disp, sizeof(disp));

				if (auto patch = FindNullsubPatch(ip, real))
					nullsubTargets.emplace_back(ip, patch);
				else
					branchTargets.emplace_back(ip);
			}
		}
	});

	uint64_t patchCount = nullsubTargets.size() + branchTargets.size();

	for (auto [ip, patch] : nullsubTargets)
	{
		uintptr_t destination = ip + *(int32_t *)(ip + 1) + 5;

		if (PatchNullsub(ip, destination, patch))
			patchCount++;
	}

	// Secondary pass to remove nullsubs missed or created above
	for (uintptr_t ip : branchTargets)
	{
		uintptr_t destination = ip + *(int32_t *)(ip + 1) + 5;

		if (PatchNullsub(ip, destination))
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

uint64_t ExperimentalPatchTemplatedFormIterator()
{
	//
	// Add a callback that sets a global variable indicating UI dropdown menu entries can be
	// deferred to prevent a redraw after every new item insert. This reduces dialog initialization time.
	//
	// The _templated_ function is designed to iterate over all FORMs of a specific type - it
	// requires hooking 100-200 separate functions in the EXE as a result. False positives are
	// a non-issue as long as ctor/dtor calls are balanced.
	//
	uint64_t patchCount = 0;
	const char *pattern = "E8 ? ? ? ? 48 89 44 24 30 48 8B 44 24 30 48 89 44 24 38 48 8B 54 24 38 48 8D 4C 24 28";

	auto matches = XUtil::FindPatterns(g_CodeBase, g_CodeEnd - g_CodeBase, pattern);

	for (uintptr_t addr : matches)
	{
		// Make sure the next call points to sub_14102CBEF (a no-op function)
		addr += 30 /* strlen(maskStr) */ + 11;
		uintptr_t destination = addr + *(int32_t *)(addr + 1) + 5;

		if (destination != OFFSET(0x102CBEF, 1530))
			continue;

		// Now look for the matching destructor call
		uintptr_t end = XUtil::FindPattern(addr, std::min<uintptr_t>(g_CodeEnd - addr, 512), "E8 ? ? ? ? 0F B6 ? ? ? 48 81 C4 ? ? ? ? C3");// sub_140FF81CE, movzx return

		if (!end)
			end = XUtil::FindPattern(addr, std::min<uintptr_t>(g_CodeEnd - addr, 512), "E8 ? ? ? ? 48 81 C4 ? ? ? ? C3");// sub_140FF81CE

		if (!end)
			continue;

		// Blacklisted (000000014148C1FF): The "Use Info" dialog has more than one list view and causes problems
		// Blacklisted (000000014169DFAD): Adding a new faction to an NPC has more than one list view
		if (addr == OFFSET(0x148C1FF, 1530) || addr == OFFSET(0x169DFAD, 1530))
			continue;

		XUtil::DetourCall(addr, &BeginUIDefer);
		XUtil::DetourCall(end, &EndUIDefer);

		patchCount += 2;
	}

	return patchCount;
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

	std::array<uint64_t, 4> counts
	{{
		ExperimentalPatchMemInit(),
		ExperimentalPatchLinkedList(),
		ExperimentalPatchTemplatedFormIterator(),
		ExperimentalPatchEditAndContinue(),
	}};

	// Then restore the old permissions
	for (auto& range : addressRanges)
	{
		Assert(VirtualProtect((void *)range.Start, range.End - range.Start, range.Protection, &range.Protection));
		Assert(FlushInstructionCache(GetCurrentProcess(), (void *)range.Start, range.End - range.Start));
	}

	auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - timerStart).count();
	LogWindow::Log("%s: (%llu + %llu + %llu + %llu) = %llu patches applied in %llums.\n", __FUNCTION__, counts[0], counts[1], counts[2], counts[3], counts[0] + counts[1] + counts[2] + counts[3], duration);
}