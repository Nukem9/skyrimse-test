#include "../../../tbb2018/scalable_allocator.h"
#include "../../common.h"
#include "MemoryManager.h"

void *Jemalloc(size_t Size, size_t Alignment = 0, bool Aligned = false, bool Zeroed = false)
{
	ProfileCounterInc("Alloc Count");
	ProfileCounterAdd("Byte Count", Size);
	ProfileTimer("Time Spent Allocating");

#if SKYRIM64_USE_VTUNE
	__itt_heap_allocate_begin(ITT_AllocateCallback, Size, Zeroed ? 1 : 0);
#endif

	if (Size == 0)
		Size = 1;

	void *ptr = nullptr;

	// Does this need to be on a certain boundary?
	if (Aligned)
	{
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

#if SKYRIM64_USE_TBBMALLOC
		ptr = scalable_aligned_malloc(Size, Alignment);
#else
		ptr = je_aligned_alloc(Alignment, Size);
#endif
	}
	else
	{
		// Normal allocation
#if SKYRIM64_USE_TBBMALLOC
		ptr = scalable_malloc(Size);
#else
		ptr = je_malloc(Size);
#endif
	}

	if (ptr && Zeroed)
		memset(ptr, 0, Size);

#if SKYRIM64_USE_VTUNE
	__itt_heap_allocate_end(ITT_AllocateCallback, &ptr, Size, Zeroed ? 1 : 0);
#endif

	return ptr;
}

void Jefree(void *Memory, bool Aligned = false)
{
	ProfileCounterInc("Free Count");
	ProfileTimer("Time Spent Freeing");

	if (!Memory)
		return;

#if SKYRIM64_USE_VTUNE
	__itt_heap_free_begin(ITT_FreeCallback, Memory);
#endif

#if SKYRIM64_USE_TBBMALLOC
	if (Aligned)
		scalable_aligned_free(Memory);
	else
		scalable_free(Memory);
#else
	je_free(Memory);
#endif

#if SKYRIM64_USE_VTUNE
	__itt_heap_free_end(ITT_FreeCallback, Memory);
#endif
}

//
// VS2015 CRT hijacked functions
//
void *__fastcall hk_calloc(size_t Count, size_t Size)
{
	// The allocated memory is always zeroed
	return Jemalloc(Count * Size, 0, false, true);
}

void *__fastcall hk_malloc(size_t Size)
{
	return Jemalloc(Size);
}

void *__fastcall hk_aligned_malloc(size_t Size, size_t Alignment)
{
	return Jemalloc(Size, Alignment, true);
}

void __fastcall hk_free(void *Block)
{
	Jefree(Block);
}

void __fastcall hk_aligned_free(void *Block)
{
	Jefree(Block, true);
}

size_t __fastcall hk_msize(void *Block)
{
#if SKYRIM64_USE_VTUNE
	__itt_heap_internal_access_begin();
#endif

#if SKYRIM64_USE_TBBMALLOC
	size_t result = scalable_msize(Block);
#else
	size_t result = je_malloc_usable_size(Block);
#endif

#if SKYRIM64_USE_VTUNE
	__itt_heap_internal_access_end();
#endif
	return result;
}

char *__fastcall hk_strdup(const char *str1)
{
	size_t len = (strlen(str1) + 1) * sizeof(char);
	return (char *)memcpy(hk_malloc(len), str1, len);
}

//
// Internal engine heap allocators backed by VirtualAlloc()
//
void *MemoryManager::Alloc(size_t Size, uint32_t Alignment, bool Aligned)
{
	return Jemalloc(Size, Alignment, Aligned, true);
}

void MemoryManager::Free(void *Memory, bool Aligned)
{
	Jefree(Memory, Aligned);
}

void *ScrapHeap::Alloc(size_t Size, uint32_t Alignment)
{
	if (Size > MAX_ALLOC_SIZE)
		return nullptr;

	return Jemalloc(Size, Alignment, Alignment != 0);
}

void ScrapHeap::Free(void *Memory)
{
	Jefree(Memory);
}

void PatchMemory()
{
#if SKYRIM64_USE_TBBMALLOC
	bool useLargePages = scalable_allocation_mode(TBBMALLOC_USE_HUGE_PAGES, 1) == TBBMALLOC_OK;
	ui::log::Add("TBBMalloc: Large pages are %s\n", useLargePages ? "enabled" : "disabled");
#endif

	PatchIAT(hk_calloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "calloc");
	PatchIAT(hk_malloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "malloc");
	PatchIAT(hk_aligned_malloc, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_aligned_malloc");
	PatchIAT(hk_free, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "free");
	PatchIAT(hk_aligned_free, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_aligned_free");
	PatchIAT(hk_msize, "API-MS-WIN-CRT-HEAP-L1-1-0.DLL", "_msize");
	PatchIAT(hk_strdup, "API-MS-WIN-CRT-STRING-L1-1-0.DLL", "_strdup");
}