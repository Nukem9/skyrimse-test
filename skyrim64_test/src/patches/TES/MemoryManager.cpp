#include "../../common.h"
#include "MemoryManager.h"

//
// VS2015 CRT hijacked functions
//
MemoryManager *fakeManager = nullptr;

void *__fastcall hk_calloc(size_t Count, size_t Size)
{
	// The allocated memory is always zeroed
	return fakeManager->Alloc(Count * Size, 0, false);
}

void *__fastcall hk_malloc(size_t Size)
{
	return fakeManager->Alloc(Size, 0, false);
}

void *__fastcall hk_aligned_malloc(size_t Size, size_t Alignment)
{
	return fakeManager->Alloc(Size, (int)Alignment, true);
}

void __fastcall hk_free(void *Block)
{
	return fakeManager->Free(Block, false);
}

void __fastcall hk_aligned_free(void *Block)
{
	return fakeManager->Free(Block, true);
}

size_t __fastcall hk_msize(void *Block)
{
	return je_malloc_usable_size(Block);
}

//
// Internal engine heap allocator backed by VirtualAlloc()
//
void *MemoryManager::Alloc(size_t Size, uint32_t Alignment, bool Aligned)
{
	ProfileCounterInc("Alloc Count");
	ProfileCounterAdd("Byte Count", Size);
	ProfileTimer("Time Spent Allocating");

	void *ptr = nullptr;

	// Does this need to be on a certain boundary?
	if (Aligned)
	{
		// Check for when the game passes in bad alignments...
		AssertMsg(Alignment != 0 && Alignment % 2 == 0, "Alignment is fucked");

		// Must be a power of 2, round it up if needed
		if ((Alignment & (Alignment - 1)) != 0)
		{
			Alignment--;
			Alignment |= Alignment >> 1;
			Alignment |= Alignment >> 2;
			Alignment |= Alignment >> 4;
			Alignment |= Alignment >> 8;
			Alignment |= Alignment >> 16;
			Alignment++;
		}

		// Size must be a multiple of alignment, round up to nearest
		if ((Size % Alignment) != 0)
			Size = ((Size + Alignment - 1) / Alignment) * Alignment;

		ptr = je_aligned_alloc(Alignment, Size);
	}
	else
	{
		// Normal allocation
		ptr = je_malloc(Size);
	}

	if (!ptr)
		return nullptr;

	return memset(ptr, 0, Size);
}

void MemoryManager::Free(void *Memory, bool Aligned)
{
	ProfileCounterInc("Free Count");
	ProfileTimer("Time Spent Freeing");

	if (!Memory)
		return;

	je_free(Memory);
}

void PatchMemory()
{
	bool option;
	option = true;  je_mallctl("background_thread", nullptr, nullptr, &option, sizeof(bool));
	option = false; je_mallctl("prof.active", nullptr, nullptr, &option, sizeof(bool));

	PatchIAT(hk_calloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "calloc");
	PatchIAT(hk_malloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "malloc");
	PatchIAT(hk_aligned_malloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_aligned_malloc");
	PatchIAT(hk_free, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "free");
	PatchIAT(hk_aligned_free, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_aligned_free");
	PatchIAT(hk_msize, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_msize");

	PatchMemory(g_ModuleBase + 0x59B560, (PBYTE)"\xC3", 1);// [3GB] MemoryManager - Default/Static/File heaps
	PatchMemory(g_ModuleBase + 0x59B170, (PBYTE)"\xC3", 1);// [1GB] BSSmallBlockAllocator
	// [128MB] BSScaleformSysMemMapper is untouched due to complexity
	// [512MB] hkMemoryAllocator is untouched due to complexity

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC01DA0), &MemoryManager::Alloc);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC020A0), &MemoryManager::Free);
}