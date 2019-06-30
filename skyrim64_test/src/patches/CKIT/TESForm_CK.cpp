#include "../../common.h"
#include <set>
#include "TESForm_CK.h"

std::unordered_map<uint64_t, void *> FormReferenceMap;
std::set<class TESForm *> AlteredFormListShadow;

void FormReferenceMap_RemoveAllEntries()
{
	for (auto[k, v] : FormReferenceMap)
	{
		if (v)
			((void(__fastcall *)(void *, int))(g_ModuleBase + 0x149F560))(v, 1);
	}

	FormReferenceMap.clear();
}

void *FormReferenceMap_FindOrCreate(uint64_t Key, bool Create)
{
	auto itr = FormReferenceMap.find(Key);

	if (itr != FormReferenceMap.end() && itr->second)
		return itr->second;

	if (Create)
	{
		void *ptr = ((void *(__fastcall *)(size_t))(g_ModuleBase + 0x1219450))(24);

		if (ptr)
			ptr = ((void *(__fastcall *)(void *))(g_ModuleBase + 0x1397CD0))(ptr);

		FormReferenceMap.insert_or_assign(Key, ptr);
		return ptr;
	}

	return nullptr;
}

void FormReferenceMap_RemoveEntry(uint64_t Key)
{
	auto itr = FormReferenceMap.find(Key);

	if (itr != FormReferenceMap.end() && itr->second)
	{
		void *ptr = itr->second;
		FormReferenceMap.erase(itr);

		((void(__fastcall *)(void *, int))(g_ModuleBase + 0x149F560))(ptr, 1);
	}
}

bool FormReferenceMap_Get(uint64_t Unused, uint64_t Key, void **Value)
{
	// Function doesn't care if entry is nullptr, only if it exists
	auto itr = FormReferenceMap.find(Key);

	if (itr != FormReferenceMap.end())
	{
		*Value = itr->second;
		return true;
	}

	return false;
}

void *AlteredFormList_Create(BSTArray<class TESForm*>* Array, uint32_t Unknown)
{
	AlteredFormListShadow.clear();

	return ((void *(__fastcall *)(BSTArray<class TESForm *> *, uint32_t))(g_ModuleBase + 0x16C6990))(Array, Unknown);
}

void AlteredFormList_RemoveAllEntries(BSTArray<class TESForm *> *Array, bool Unknown)
{
	AlteredFormListShadow.clear();

	((void(__fastcall *)(BSTArray<class TESForm *> *, bool))(g_ModuleBase + 0x139B2B0))(Array, Unknown);
}

void AlteredFormList_Insert(BSTArray<class TESForm *> *Array, class TESForm *&Entry)
{
	AlteredFormListShadow.insert(Entry);

	((void(__fastcall *)(BSTArray<class TESForm *> *, class TESForm *&))(g_ModuleBase + 0x146A660))(Array, Entry);
}

void AlteredFormList_RemoveEntry(BSTArray<class TESForm *> *Array, uint32_t Index, uint32_t Unknown)
{
	AlteredFormListShadow.erase(Array->at(Index));

	((void(__fastcall *)(BSTArray<class TESForm *> *, uint32_t, uint32_t))(g_ModuleBase + 0x165EA50))(Array, Index, Unknown);
}

bool AlteredFormList_ElementExists(BSTArray<class TESForm *> *Array, class TESForm *&Entry)
{
	return AlteredFormListShadow.count(Entry) > 0;
}
}