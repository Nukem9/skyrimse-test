#include <tbb/concurrent_hash_map.h>
#include "../../common.h"
#include "BSTScatterTable.h"
#include "BSReadWriteLock.h"
#include "TESForm.h"
#include "BGSDistantTreeBlock.h"
#include "MemoryManager.h"

AutoPtr(bool, byte_141EE9B98, 0x1EE9B98);

AutoPtr(BSReadWriteLock, GlobalFormLock, 0x1EEA0D0);
AutoPtr(templated(BSTCRCScatterTable<uint32_t, TESForm *> *), GlobalFormList, 0x1EE9C38);

tbb::concurrent_hash_map<uint32_t, TESForm *> g_FormMap[TES_FORM_MASTER_COUNT];
tbb::concurrent_hash_map<uint32_t, const char *> g_EditorNameMap;

// The form list is maintained at the end of this file
struct FormEnumEntry
{
	const char *Name;
	uint32_t NameAsInt;
	uint32_t Unknown1;
	uint32_t TypeId;
	uint32_t Unknown2;
};

extern const FormEnumEntry FormEnum[138];

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

void TESForm::hk_GetFullTypeName(char *Buffer, uint32_t BufferSize)
{
	_snprintf_s(Buffer, _TRUNCATE, BufferSize, "%s Form '%s' (%08X)", FormEnum[GetType()].Name, GetName(), GetType());
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

const char *TESObjectREFR::hk_GetName()
{
	if (!byte_141EE9B98)
	{
		byte_141EE9B98 = true;
		const char *name = GetName();
		byte_141EE9B98 = false;

		if (name && strlen(name) > 0)
			return name;
	}

	TESForm *base = GetBaseObject();

	// This checks some unknown form manager instance
	if (!base /* || !qword_141EE43A8 || *(_BYTE *)(qword_141EE43A8 + 0xDA5) */)
		return "";

	const char *name = base->GetName();

	if (name && strlen(name) > 0)
		return name;

	// Now it tries fetching any ExtraData, but I don't care right now
	return "";
}

std::vector<TESForm *> TESForm::LookupFormsByType(uint32_t Type, bool SortById, bool SortByName)
{
	std::vector<TESForm *> data;

	GlobalFormLock.LockForRead();
	{
		if (GlobalFormList)
		{
			for (auto itr = GlobalFormList->begin(); itr != GlobalFormList->end(); itr++)
			{
				if (itr->GetType() == Type)
					data.push_back(*itr);
			}
		}
	}
	GlobalFormLock.UnlockRead();

	if (SortById)
	{
		std::sort(data.begin(), data.end(),
			[](TESForm *& a, TESForm *& b) -> bool
		{
			return a->GetId() < b->GetId();
		});
	}
	else if (SortByName)
	{
		std::sort(data.begin(), data.end(),
			[](TESForm *& a, TESForm *& b) -> bool
		{
			return strcmp(a->GetName(), b->GetName()) < 0;
		});
	}

	return data;
}

void CRC32_Lazy(int *out, int idIn)
{
	AutoFunc(void(*)(int *, int), sub_140C06030, 0xC06030);
	sub_140C06030(out, idIn);
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

#pragma region Form List
//
// Skyrim:   137 form types
// SkyrimSE: 138 form types
//
// Notes:
// - NameAsInt is generated at runtime
// - VOLI is new with a special index (unknown)
//
const FormEnumEntry FormEnum[138] =
{
  { "NONE", 0, 0, 1, 0 },
  { "TES4", 0, 0, 2, 0 },
  { "GRUP", 0, 0, 3, 0 },
  { "GMST", 0, 0, 4, 0 },
  { "KYWD", 0, 0, 5, 0 },
  { "LCRT", 0, 0, 6, 0 },
  { "AACT", 0, 0, 7, 0 },
  { "TXST", 0, 0, 8, 0 },
  { "MICN", 0, 0, 9, 0 },
  { "GLOB", 0, 0, 10, 0 },
  { "CLAS", 0, 0, 11, 0 },
  { "FACT", 0, 0, 12, 0 },
  { "HDPT", 0, 0, 13, 0 },
  { "EYES", 0, 0, 14, 0 },
  { "RACE", 0, 0, 15, 0 },
  { "SOUN", 0, 0, 16, 0 },
  { "ASPC", 0, 0, 17, 0 },
  { "SKIL", 0, 0, 18, 0 },
  { "MGEF", 0, 0, 19, 0 },
  { "SCPT", 0, 0, 20, 0 },
  { "LTEX", 0, 0, 21, 0 },
  { "ENCH", 0, 0, 22, 0 },
  { "SPEL", 0, 0, 23, 0 },
  { "SCRL", 0, 0, 24, 0 },
  { "ACTI", 0, 0, 25, 0 },
  { "TACT", 0, 0, 26, 0 },
  { "ARMO", 0, 0, 27, 0 },
  { "BOOK", 0, 0, 28, 0 },
  { "CONT", 0, 0, 29, 0 },
  { "DOOR", 0, 0, 30, 0 },
  { "INGR", 0, 0, 31, 0 },
  { "LIGH", 0, 0, 32, 0 },
  { "MISC", 0, 0, 33, 0 },
  { "APPA", 0, 0, 34, 0 },
  { "STAT", 0, 0, 35, 0 },
  { "SCOL", 0, 0, 36, 0 },
  { "MSTT", 0, 0, 37, 0 },
  { "GRAS", 0, 0, 38, 0 },
  { "TREE", 0, 0, 39, 0 },
  { "FLOR", 0, 0, 40, 0 },
  { "FURN", 0, 0, 41, 0 },
  { "WEAP", 0, 0, 42, 0 },
  { "AMMO", 0, 0, 43, 0 },
  { "NPC_", 0, 0, 44, 0 },
  { "LVLN", 0, 0, 45, 0 },
  { "KEYM", 0, 0, 46, 0 },
  { "ALCH", 0, 0, 47, 0 },
  { "IDLM", 0, 0, 48, 0 },
  { "NOTE", 0, 0, 49, 0 },
  { "COBJ", 0, 0, 50, 0 },
  { "PROJ", 0, 0, 51, 0 },
  { "HAZD", 0, 0, 52, 0 },
  { "SLGM", 0, 0, 53, 0 },
  { "LVLI", 0, 0, 54, 0 },
  { "WTHR", 0, 0, 55, 0 },
  { "CLMT", 0, 0, 56, 0 },
  { "SPGD", 0, 0, 57, 0 },
  { "RFCT", 0, 0, 58, 0 },
  { "REGN", 0, 0, 59, 0 },
  { "NAVI", 0, 0, 60, 0 },
  { "CELL", 0, 0, 61, 0 },
  { "REFR", 0, 0, 62, 0 },
  { "ACHR", 0, 0, 63, 0 },
  { "PMIS", 0, 0, 64, 0 },
  { "PARW", 0, 0, 65, 0 },
  { "PGRE", 0, 0, 66, 0 },
  { "PBEA", 0, 0, 67, 0 },
  { "PFLA", 0, 0, 68, 0 },
  { "PCON", 0, 0, 69, 0 },
  { "PBAR", 0, 0, 70, 0 },
  { "PHZD", 0, 0, 71, 0 },
  { "WRLD", 0, 0, 72, 0 },
  { "LAND", 0, 0, 73, 0 },
  { "NAVM", 0, 0, 74, 0 },
  { "TLOD", 0, 0, 75, 0 },
  { "DIAL", 0, 0, 76, 0 },
  { "INFO", 0, 0, 77, 0 },
  { "QUST", 0, 0, 78, 0 },
  { "IDLE", 0, 0, 79, 0 },
  { "PACK", 0, 0, 80, 0 },
  { "CSTY", 0, 0, 81, 0 },
  { "LSCR", 0, 0, 82, 0 },
  { "LVSP", 0, 0, 83, 0 },
  { "ANIO", 0, 0, 84, 0 },
  { "WATR", 0, 0, 85, 0 },
  { "EFSH", 0, 0, 86, 0 },
  { "TOFT", 0, 0, 87, 0 },
  { "EXPL", 0, 0, 88, 0 },
  { "DEBR", 0, 0, 89, 0 },
  { "IMGS", 0, 0, 90, 0 },
  { "IMAD", 0, 0, 91, 0 },
  { "FLST", 0, 0, 92, 0 },
  { "PERK", 0, 0, 93, 0 },
  { "BPTD", 0, 0, 94, 0 },
  { "ADDN", 0, 0, 95, 0 },
  { "AVIF", 0, 0, 96, 0 },
  { "CAMS", 0, 0, 97, 0 },
  { "CPTH", 0, 0, 98, 0 },
  { "VTYP", 0, 0, 99, 0 },
  { "MATT", 0, 0, 100, 0 },
  { "IPCT", 0, 0, 101, 0 },
  { "IPDS", 0, 0, 102, 0 },
  { "ARMA", 0, 0, 103, 0 },
  { "ECZN", 0, 0, 104, 0 },
  { "LCTN", 0, 0, 105, 0 },
  { "MESG", 0, 0, 106, 0 },
  { "RGDL", 0, 0, 107, 0 },
  { "DOBJ", 0, 0, 108, 0 },
  { "LGTM", 0, 0, 109, 0 },
  { "MUSC", 0, 0, 110, 0 },
  { "FSTP", 0, 0, 111, 0 },
  { "FSTS", 0, 0, 112, 0 },
  { "SMBN", 0, 0, 113, 0 },
  { "SMQN", 0, 0, 114, 0 },
  { "SMEN", 0, 0, 115, 0 },
  { "DLBR", 0, 0, 116, 0 },
  { "MUST", 0, 0, 117, 0 },
  { "DLVW", 0, 0, 118, 0 },
  { "WOOP", 0, 0, 119, 0 },
  { "SHOU", 0, 0, 120, 0 },
  { "EQUP", 0, 0, 121, 0 },
  { "RELA", 0, 0, 122, 0 },
  { "SCEN", 0, 0, 123, 0 },
  { "ASTP", 0, 0, 124, 0 },
  { "OTFT", 0, 0, 125, 0 },
  { "ARTO", 0, 0, 126, 0 },
  { "MATO", 0, 0, 127, 0 },
  { "MOVT", 0, 0, 128, 0 },
  { "SNDR", 0, 0, 129, 0 },
  { "DUAL", 0, 0, 130, 0 },
  { "SNCT", 0, 0, 131, 0 },
  { "SOPM", 0, 0, 132, 0 },
  { "COLL", 0, 0, 133, 0 },
  { "CLFM", 0, 0, 134, 0 },
  { "REVB", 0, 0, 135, 0 },
  { "LENS", 0, 0, 136, 0 },
  { "LSPR", 0, 0, 137, 0 },
  { "VOLI", 0, 0, 0x41555328, 1 },
};
#pragma endregion