#include "../../../tbb2018/concurrent_unordered_map.h"
#include "../../common.h"
#include "BSTScatterTable.h"
#include "BSReadWriteLock.h"
#include "TESForm.h"

AutoPtr(BSReadWriteLock, GlobalFormLock, 0x1EEA0D0);
AutoPtr(templated(BSTCRCScatterTable<uint32_t, TESForm *> *), GlobalFormList, 0x1EE9C38);

tbb::concurrent_unordered_map<uint32_t, TESForm *> g_FormMap[TES_FORM_MASTER_COUNT];

namespace Bitmap
{
    const uint32_t EntrySize  = (TES_FORM_INDEX_COUNT / 32) * sizeof(LONG);
    const uint32_t EntryCount = TES_FORM_MASTER_COUNT;

	alignas(64) LONG *FormBitmap[EntryCount];

	uintptr_t MemorySize;
	uintptr_t MemoryBase;
	uintptr_t MemoryEnd;

	LONG CALLBACK PageFaultExceptionFilter(LPEXCEPTION_POINTERS Info)
	{
		// Check if the violation was inside our memory region
		uintptr_t code = Info->ExceptionRecord->ExceptionCode;
		uintptr_t addr = Info->ExceptionRecord->ExceptionInformation[1];

		if (code != EXCEPTION_ACCESS_VIOLATION)
			return EXCEPTION_CONTINUE_SEARCH;

		if (addr < MemoryBase || addr > MemoryEnd)
			return EXCEPTION_CONTINUE_SEARCH;

		// Lazy-allocate the bitmap when needed (128KB [aligned] chunks, prevent committing unused pages)
		uintptr_t sliceSize = 128 * 1024;
		uintptr_t sliceBase = (addr / sliceSize) * sliceSize;

		sliceBase = max(sliceBase, MemoryBase);
		sliceSize = min(sliceSize, MemoryEnd - sliceBase);

		VirtualAlloc((LPVOID)sliceBase, sliceSize, MEM_COMMIT, PAGE_READWRITE);
		return EXCEPTION_CONTINUE_EXECUTION;
	}

    void TryInitRange()
    {
        MemorySize = EntryCount * EntrySize;
        MemoryBase = (uintptr_t)VirtualAlloc(nullptr, MemorySize, MEM_RESERVE, PAGE_NOACCESS);
		MemoryEnd  = MemoryBase + MemorySize - 1;

        for (int i = 0; i < EntryCount; i++)
            FormBitmap[i] = (LONG *)(MemoryBase + (i * EntrySize));

		AddVectoredExceptionHandler(0, PageFaultExceptionFilter);
    }

#define WORD_OFFSET(b) ((b) / 32)
#define BIT_OFFSET(b) ((b) % 32)

	void SetNull(uint32_t MasterId, uint32_t BaseId)
    {
        if (MasterId >= EntryCount)
            return;

		_interlockedbittestandset(&FormBitmap[MasterId][WORD_OFFSET(BaseId)], BIT_OFFSET(BaseId));
    }

	void Unset(uint32_t MasterId, uint32_t BaseId)
    {
        if (MasterId >= EntryCount)
            return;

		_interlockedbittestandreset(&FormBitmap[MasterId][WORD_OFFSET(BaseId)], BIT_OFFSET(BaseId));
    }

	bool IsNull(uint32_t MasterId, uint32_t BaseId)
    {
        if (MasterId >= EntryCount)
            return false;

        // If bit is set, return true
        return _bittest(&FormBitmap[MasterId][WORD_OFFSET(BaseId)], BIT_OFFSET(BaseId)) != 0;
    }

#undef WORD_OFFSET
#undef BIT_OFFSET
}

void UpdateFormCache(uint32_t FormId, TESForm *Value, bool Invalidate)
{
    ProfileTimer("Cache Update Time");

    const unsigned char masterId = (FormId & 0xFF000000) >> 24;
	const unsigned int baseId	 = (FormId & 0x00FFFFFF);

    // NOTE: If the pointer is 0 we can short-circuit and skip the entire
    // hash map. Any fetches will check the bitmap first and also skip the map.
    if (!Value && !Invalidate)
    {
        // Atomic write can be outside of the lock
        Bitmap::SetNull(masterId, baseId);
    }
	else
	{
		if (Invalidate)
			Bitmap::Unset(masterId, baseId);

		if (Invalidate)
			g_FormMap[masterId][baseId] = (TESForm *)1;
		else
			g_FormMap[masterId][baseId] = Value;
	}
}

bool GetFormCache(uint32_t FormId, TESForm *&Form)
{
	if (!ui::opt::EnableCache)
		return false;

    ProfileCounterInc("Cache Lookups");
    ProfileTimer("Cache Fetch Time");

    const unsigned char masterId = (FormId & 0xFF000000) >> 24;
    const unsigned int baseId    = (FormId & 0x00FFFFFF);
    bool hit;

    // Check if the bitmap says this is a nullptr
    if (Bitmap::IsNull(masterId, baseId))
    {
        hit  = true;
        Form = nullptr;

        ProfileCounterInc("Null Fetches");
    }
	else
	{
		// Slow method: Check if the hashmap entry exists
		const auto &map = g_FormMap[masterId];

		if (auto e = map.find(baseId); e != map.end() && e->second != (TESForm *)1)
		{
			hit = true;
			Form = e->second;
		}
		else
		{
			// Total cache miss, worst case scenario
			hit = false;
			Form = nullptr;

			ProfileCounterInc("Cache Misses");
		}
	}

    return hit;
}

void CRC32_Lazy(int *out, int idIn)
{
    ((void (*)(int *, int))(g_ModuleBase + 0xC06030))(out, idIn);
}

TESForm *GetFormById(unsigned int FormId)
{
    // Hybrid bitmap/std::unordered_map cache
	TESForm *formPointer;

	if (GetFormCache(FormId, formPointer))
        return formPointer;

    // Try to use Bethesda's scatter table which is considerably slower
	GlobalFormLock.LockForRead();

	if (!GlobalFormList || !GlobalFormList->get(FormId, formPointer))
		formPointer = nullptr;

	GlobalFormLock.UnlockRead();

    UpdateFormCache(FormId, formPointer, false);
    return formPointer;
}

uint8_t *origFunc3 = nullptr;
__int64 UnknownFormFunction3(__int64 a1, __int64 a2, int a3, __int64 a4)
{
    UpdateFormCache(*(uint32_t *)a4, nullptr, true);

    return ((decltype(&UnknownFormFunction3))origFunc3)(a1, a2, a3, a4);
}

uint8_t *origFunc2 = nullptr;
__int64 UnknownFormFunction2(__int64 a1, __int64 a2, int a3, DWORD *formId, __int64 **a5)
{
    UpdateFormCache(*formId, nullptr, true);

    return ((decltype(&UnknownFormFunction2))origFunc2)(a1, a2, a3, formId, a5);
}

uint8_t *origFunc1 = nullptr;
__int64 UnknownFormFunction1(__int64 a1, __int64 a2, int a3, DWORD *formId, __int64 *a5)
{
    UpdateFormCache(*formId, nullptr, true);

    return ((decltype(&UnknownFormFunction1))origFunc1)(a1, a2, a3, formId, a5);
}

uint8_t *origFunc0 = nullptr;
void UnknownFormFunction0(__int64 form, bool a2)
{
    UpdateFormCache(*(uint32_t *)(form + 0x14), nullptr, true);

	((decltype(&UnknownFormFunction0))origFunc0)(form, a2);
}

void PatchTESForm()
{
    Bitmap::TryInitRange();

	origFunc0 = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x194970), &UnknownFormFunction0);
	origFunc1 = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x196070), &UnknownFormFunction1);
	origFunc2 = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x195DA0), &UnknownFormFunction2);
	origFunc3 = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x196960), &UnknownFormFunction3);
    Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1943B0), &GetFormById);
}
