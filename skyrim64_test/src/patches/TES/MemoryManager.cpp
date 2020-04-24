#include "../../common.h"
#include "MemoryManager.h"

void *MemAlloc(size_t Size, size_t Alignment = 0, bool Aligned = false, bool Zeroed = false)
{
	ProfileCounterInc("Alloc Count");
	ProfileCounterAdd("Byte Count", Size);
	ProfileTimer("Time Spent Allocating");

#if SKYRIM64_USE_VTUNE
	__itt_heap_allocate_begin(ITT_AllocateCallback, Size, Zeroed ? 1 : 0);
#endif

	// If the caller doesn't care, force 4 byte aligns as a minimum
	if (!Aligned)
		Alignment = 4;

	if (Size <= 0)
	{
		Size = 1;
		Alignment = 2;
	}

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

#if SKYRIM64_USE_PAGE_HEAP
	void *ptr = VirtualAlloc(nullptr, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	void *ptr = scalable_aligned_malloc(Size, Alignment);

	if (ptr && Zeroed)
		memset(ptr, 0, Size);
#endif

#if SKYRIM64_USE_VTUNE
	__itt_heap_allocate_end(ITT_AllocateCallback, &ptr, Size, Zeroed ? 1 : 0);
#endif

	return ptr;
}

void MemFree(void *Memory, bool Aligned = false)
{
	ProfileCounterInc("Free Count");
	ProfileTimer("Time Spent Freeing");

	if (!Memory)
		return;
	
#if SKYRIM64_USE_VTUNE
	__itt_heap_free_begin(ITT_FreeCallback, Memory);
#endif

#if SKYRIM64_USE_PAGE_HEAP
	VirtualFree(Memory, 0, MEM_RELEASE);
#else
	scalable_aligned_free(Memory);
#endif

#if SKYRIM64_USE_VTUNE
	__itt_heap_free_end(ITT_FreeCallback, Memory);
#endif
}

size_t MemSize(void *Memory)
{
#if SKYRIM64_USE_VTUNE
	__itt_heap_internal_access_begin();
#endif

#if SKYRIM64_USE_PAGE_HEAP
	MEMORY_BASIC_INFORMATION info;
	VirtualQuery(Memory, &info, sizeof(MEMORY_BASIC_INFORMATION));

	size_t result = info.RegionSize;
#else
	size_t result = scalable_msize(Memory);
#endif

#if SKYRIM64_USE_VTUNE
	__itt_heap_internal_access_end();
#endif

	return result;
}

//
// VS2015 CRT hijacked functions
//
void *hk_calloc(size_t Count, size_t Size)
{
	// The allocated memory is always zeroed
	return MemAlloc(Count * Size, 0, false, true);
}

void *hk_malloc(size_t Size)
{
	return MemAlloc(Size);
}

void *hk_aligned_malloc(size_t Size, size_t Alignment)
{
	return MemAlloc(Size, Alignment, true);
}

void *hk_realloc(void *Memory, size_t Size)
{
	void *newMemory = nullptr;

	if (Size > 0)
	{
		// Recalloc behaves like calloc if there's no existing allocation. Realloc doesn't. Zero it either way.
		newMemory = MemAlloc(Size, 0, false, true);

		if (Memory)
			memcpy(newMemory, Memory, std::min(Size, MemSize(Memory)));
	}

	MemFree(Memory);
	return newMemory;
}

void *hk_recalloc(void *Memory, size_t Count, size_t Size)
{
	return hk_realloc(Memory, Count * Size);
}

void hk_free(void *Block)
{
	MemFree(Block);
}

void hk_aligned_free(void *Block)
{
	MemFree(Block, true);
}

size_t hk_msize(void *Block)
{
	return MemSize(Block);
}

char *hk_strdup(const char *str1)
{
	size_t len = (strlen(str1) + 1) * sizeof(char);
	return (char *)memcpy(hk_malloc(len), str1, len);
}

//
// Internal engine heap allocators backed by VirtualAlloc()
//
void *MemoryManager::Allocate(MemoryManager *Manager, size_t Size, uint32_t Alignment, bool Aligned)
{
	return MemAlloc(Size, Alignment, Aligned, true);
}

void MemoryManager::Deallocate(MemoryManager *Manager, void *Memory, bool Aligned)
{
	MemFree(Memory, Aligned);
}

size_t MemoryManager::Size(MemoryManager *Manager, void *Memory)
{
	return MemSize(Memory);
}

void *ScrapHeap::Allocate(size_t Size, uint32_t Alignment)
{
	if (Size > MAX_ALLOC_SIZE)
		return nullptr;

	return MemAlloc(Size, Alignment, Alignment != 0);
}

void ScrapHeap::Deallocate(void *Memory)
{
	MemFree(Memory);
}

void PatchMemory()
{
	scalable_allocation_mode(TBBMALLOC_USE_HUGE_PAGES, 1);

	PatchIAT(hk_calloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "calloc");
	PatchIAT(hk_malloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "malloc");
	PatchIAT(hk_aligned_malloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_aligned_malloc");
	PatchIAT(hk_recalloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_recalloc");
	PatchIAT(hk_free, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "free");
	PatchIAT(hk_aligned_free, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_aligned_free");
	PatchIAT(hk_msize, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_msize");
	PatchIAT(hk_strdup, "API-MS-WIN-CRT-STRING-L1-1-0.DLL", "_strdup");

	PatchIAT(hk_calloc, "MSVCR110.dll", "calloc");
	PatchIAT(hk_malloc, "MSVCR110.dll", "malloc");
	PatchIAT(hk_aligned_malloc, "MSVCR110.dll", "_aligned_malloc");
	PatchIAT(hk_realloc, "MSVCR110.dll", "realloc");
	PatchIAT(hk_free, "MSVCR110.dll", "free");
	PatchIAT(hk_aligned_free, "MSVCR110.dll", "_aligned_free");
	PatchIAT(hk_msize, "MSVCR110.dll", "_msize");
	PatchIAT(hk_strdup, "MSVCR110.dll", "_strdup");
}