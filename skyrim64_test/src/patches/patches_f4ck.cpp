#include "../common.h"
#include <libdeflate/libdeflate.h>
#include "TES/MemoryManager.h"
#include "TES/BSTArray.h"
#include "TES/bhkThreadMemorySource.h"

#pragma comment(lib, "libdeflate.lib")

struct z_stream_s
{
	const void *next_in;
	uint32_t avail_in;
	uint32_t total_in;
	void *next_out;
	uint32_t avail_out;
	uint32_t total_out;
	const char *msg;
	struct internal_state *state;
};
static_assert_offset(z_stream_s, state, 0x28);

int hk_inflateInit(z_stream_s *Stream, const char *Version, int Mode)
{
	// Force inflateEnd to error out and skip frees
	Stream->state = nullptr;

	return 0;
}

int hk_inflate(z_stream_s *Stream, int Flush)
{
	size_t outBytes = 0;
	libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();

	libdeflate_result result = libdeflate_zlib_decompress(decompressor, Stream->next_in, Stream->avail_in, Stream->next_out, Stream->avail_out, &outBytes);
	libdeflate_free_decompressor(decompressor);

	if (result == LIBDEFLATE_SUCCESS)
	{
		Assert(outBytes < std::numeric_limits<uint32_t>::max());

		Stream->total_in = Stream->avail_in;
		Stream->total_out = (uint32_t)outBytes;

		return 1;
	}

	if (result == LIBDEFLATE_INSUFFICIENT_SPACE)
		return -5;

	return -2;
}

void PatchMemory();

uint32_t sub_1414974E0(BSTArray<void *>& Array, const void *&Target, uint32_t StartIndex, __int64 Unused)
{
	for (uint32_t i = StartIndex; i < Array.QSize(); i++)
	{
		if (Array[i] == Target)
			return i;
	}

	return 0xFFFFFFFF;
}

uint32_t sub_1414974E0_SSE41(BSTArray<void *>& Array, const void *&Target, uint32_t StartIndex, __int64 Unused)
{
	const uint32_t size = Array.QSize();
	const __int64 *data = (const __int64 *)Array.QBuffer();

	const uint32_t comparesPerIter = 4;
	const uint32_t alignedSize = size & ~(comparesPerIter - 1);

	// Compare 4 pointers per iteration (Strips off the last 2 bits from array size)
	const __m128i targets = _mm_set1_epi64x((__int64)Target);

	for (uint32_t i = StartIndex; i < alignedSize; i += comparesPerIter)
	{
		//
		// Set bit 0 if 'a1'=='a2', set bit 1 if 'b1'=='b2', set bit X...
		// AVX: mask = _mm256_movemask_pd(_mm256_castsi256_pd(_mm256_cmpeq_epi64(targets, _mm256_loadu_si256((__m256i *)&data[i]))));
		//
		__m128i test1 = _mm_cmpeq_epi64(targets, _mm_loadu_si128((__m128i *) & data[i + 0]));
		__m128i test2 = _mm_cmpeq_epi64(targets, _mm_loadu_si128((__m128i *) & data[i + 2]));

		int mask = _mm_movemask_pd(_mm_castsi128_pd(_mm_or_si128(test1, test2)));

		if (mask != 0)
		{
			for (; i < size; i++)
			{
				if (data[i] == (__int64)Target)
					return i;
			}

			__debugbreak();
			__assume(0);
		}
	}

	for (uint32_t i = alignedSize; i < size; i++)
	{
		if (data[i] == (__int64)Target)
			return i;
	}

	return 0xFFFFFFFF;
}

void Patch_Fallout4CreationKit()
{
	//
	// MemoryManager
	//
	//if (g_INI.GetBoolean("CreationKit", "MemoryPatch", false))
	{
		PatchMemory();

		XUtil::PatchMemory(g_ModuleBase + 0x030ECC0, (PBYTE)"\xC3", 1);					// [3GB  ] MemoryManager - Default/Static/File heaps
		XUtil::PatchMemory(g_ModuleBase + 0x2004B70, (PBYTE)"\xC3", 1);					// [1GB  ] BSSmallBlockAllocator
		XUtil::DetourJump(g_ModuleBase + 0x21115D0, &bhkThreadMemorySource::__ctor__);	// [512MB] bhkThreadMemorySource
		XUtil::PatchMemory(g_ModuleBase + 0x200A920, (PBYTE)"\xC3", 1);					// [64MB ] ScrapHeap init
		XUtil::PatchMemory(g_ModuleBase + 0x200B440, (PBYTE)"\xC3", 1);					// [64MB ] ScrapHeap deinit
																						// [128MB] BSScaleformSysMemMapper is untouched due to complexity

		XUtil::DetourJump(g_ModuleBase + 0x2004E20, &MemoryManager::Allocate);
		XUtil::DetourJump(g_ModuleBase + 0x20052D0, &MemoryManager::Deallocate);
		XUtil::DetourJump(g_ModuleBase + 0x2004300, &MemoryManager::Size);
		XUtil::DetourJump(g_ModuleBase + 0x200AB30, &ScrapHeap::Allocate);
		XUtil::DetourJump(g_ModuleBase + 0x200B170, &ScrapHeap::Deallocate);
	}

	XUtil::DetourCall(g_ModuleBase + 0x08056B7, &hk_inflateInit);
	XUtil::DetourCall(g_ModuleBase + 0x08056F7, &hk_inflate);

	int cpuinfo[4];
	__cpuid(cpuinfo, 1);

	// Utilize SSE4.1 instructions if available
	if ((cpuinfo[2] & (1 << 19)) != 0)
		XUtil::DetourJump(g_ModuleBase + 0x05B31C0, &sub_1414974E0_SSE41);
	else
		XUtil::DetourJump(g_ModuleBase + 0x05B31C0, &sub_1414974E0);
}