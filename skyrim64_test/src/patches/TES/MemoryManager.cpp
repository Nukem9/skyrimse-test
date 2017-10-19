#include "../../stdafx.h"

__int64 allocCount = 0;
__int64 byteCount = 0;
__int64 freeCount = 0;

__int64 timeSpentAllocating = 0;
__int64 timeSpentFreeing = 0;

//
// Kill the original heap allocations
//
LPVOID WINAPI hk_VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
	// Kill MemoryHeap
	static PVOID tracker = nullptr;

	if ((dwSize == 0x80000000 || dwSize == 0x20000000) || (lpAddress && lpAddress == tracker))
	{
		if (!tracker)
			tracker = VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		return tracker;
	}

	// Kill BSSmallBlockAllocator
	if (dwSize > (900 * 1024 * 1024))
		return nullptr;

	// Kill hkMemoryAllocator
	if (dwSize > (500 * 1024 * 1024))
	{
		*(DWORD *)((g_ModuleBase + 0x2FC4090 + 8) + 0xB8) = 0x40000;
		dwSize = 0x40000 + 0x40000;
	}

	return VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}

//
// VS2015 CRT hijacked functions
//
void *__fastcall hk_calloc(size_t Count, size_t Size)
{
	// The allocated memory is always zeroed
	return Scaleform_HeapAlloc(nullptr, Count * Size, 0, false);
}

void *__fastcall hk_malloc(size_t Size)
{
	return Scaleform_HeapAlloc(nullptr, Size, 0, false);
}

void *__fastcall hk_aligned_malloc(size_t Size, size_t Alignment)
{
	return Scaleform_HeapAlloc(nullptr, Size, (int)Alignment, true);
}

void __fastcall hk_free(void *Block)
{
	return Scaleform_HeapFree(nullptr, Block, false);
}

void __fastcall hk_aligned_free(void *Block)
{
	return Scaleform_HeapFree(nullptr, Block, true);
}

size_t __fastcall hk_msize(void *Block)
{
	return je_malloc_usable_size(Block);
}

//
// Internal engine heap allocator backed by VirtualAlloc()
//
void *Scaleform_HeapAlloc(void *Heap, size_t Size, unsigned int Alignment, bool Aligned)
{
	ProfileCounterInc("Alloc Count");
	ProfileCounterAdd("Byte Count", Size);
	ProfileTimer("Time Spent Allocating");

	void *ptr = nullptr;

	// Does this need to be on a certain boundary?
	if (Aligned)
	{
		// Check for when the game passes in bad alignments...
		if (Alignment == 0 || Alignment % 2 != 0)
			MessageBoxA(nullptr, "Alignment is fucked", "", 0);

		// Alignment must be a power of 2, round it up if needed
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

void Scaleform_HeapFree(void *Heap, void *Memory, bool Aligned)
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

	PatchIAT(hk_VirtualAlloc, "kernel32.dll", "VirtualAlloc");

	PatchIAT(hk_calloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "calloc");
	PatchIAT(hk_malloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "malloc");
	PatchIAT(hk_aligned_malloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_aligned_malloc");
	PatchIAT(hk_free, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "free");
	PatchIAT(hk_aligned_free, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_aligned_free");
	PatchIAT(hk_msize, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_msize");

	Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xBE3E90), (PBYTE)&Scaleform_HeapAlloc, Detours::X64Option::USE_PUSH_RET);
	Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xBE4190), (PBYTE)&Scaleform_HeapFree, Detours::X64Option::USE_PUSH_RET);
}