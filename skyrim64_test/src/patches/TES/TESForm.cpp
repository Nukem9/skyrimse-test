#include "../../../tbb2018/concurrent_hash_map.h"
#include "../../common.h"
#include "BSTScatterTable.h"
#include "BSReadWriteLock.h"
#include "TESForm.h"
#include "BGSDistantTreeBlock.h"
#include "MemoryManager.h"

AutoPtr(BSReadWriteLock, GlobalFormLock, 0x1EEA0D0);
AutoPtr(templated(BSTCRCScatterTable<uint32_t, TESForm *> *), GlobalFormList, 0x1EE9C38);

tbb::concurrent_hash_map<uint32_t, TESForm *> g_FormMap[TES_FORM_MASTER_COUNT];
tbb::concurrent_hash_map<uint32_t, const char *> g_EditorNameMap;

void UpdateFormCache(uint32_t FormId, TESForm *Value, bool Invalidate)
{
	ProfileTimer("Cache Update Time");

	const unsigned char masterId = (FormId & 0xFF000000) >> 24;
	const unsigned int baseId = (FormId & 0x00FFFFFF);

	if (Invalidate)
		g_FormMap[masterId].erase(baseId);
	else
		g_FormMap[masterId].insert(std::make_pair(baseId, Value));

	BGSDistantTreeBlock::InvalidateCachedForm(FormId);
}

bool GetFormCache(uint32_t FormId, TESForm *&Form)
{
	if (!ui::opt::EnableCache)
		return false;

	ProfileCounterInc("Cache Lookups");
	ProfileTimer("Cache Fetch Time");

	const unsigned char masterId = (FormId & 0xFF000000) >> 24;
	const unsigned int baseId = (FormId & 0x00FFFFFF);

	// Is it present in our map?
	{
		tbb::concurrent_hash_map<uint32_t, TESForm *>::accessor accessor;

		if (g_FormMap[masterId].find(accessor, baseId))
		{
			Form = accessor->second;
			return true;
		}
	}

	// Cache miss: worst case scenario
	ProfileCounterInc("Cache Misses");

	Form = nullptr;
	return false;
}

const char *TESForm::hk_GetName()
{
	tbb::concurrent_hash_map<uint32_t, const char *>::accessor accessor;

	if (g_EditorNameMap.find(accessor, GetId()))
		return accessor->second;

	// By default Skyrim returns an empty string
	return "";
}

bool TESForm::hk_SetEditorId(const char *Name)
{
	size_t len = strlen(Name) + 1;
	char *data = (char *)MemoryManager::Alloc(nullptr, len, 0, false);
	strcpy_s(data, len, Name);

	g_EditorNameMap.insert(std::make_pair(GetId(), data));
	return true;
}

TESForm *TESForm::LookupFormById(uint32_t FormId)
{
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

void CRC32_Lazy(int *out, int idIn)
{
	((void(*)(int *, int))(g_ModuleBase + 0xC06030))(out, idIn);
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
	origFunc0 = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x194970), &UnknownFormFunction0);
	origFunc1 = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x196070), &UnknownFormFunction1);
	origFunc2 = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x195DA0), &UnknownFormFunction2);
	origFunc3 = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x196960), &UnknownFormFunction3);
    Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1943B0), &TESForm::LookupFormById);
}