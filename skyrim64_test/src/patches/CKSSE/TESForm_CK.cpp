#include "../../common.h"
#include <set>
#include "TESForm_CK.h"

std::unordered_map<uint64_t, void *> FormReferenceMap;
std::set<TESForm_CK *> AlteredFormListShadow;

bool TESForm_CK::GetActive() const
{
	return (FormFlags & 2) != 0;
}

uint32_t TESForm_CK::GetFormID() const
{
	return FormID;
}

TESForm_CK *TESForm_CK::GetFormByNumericID(uint32_t SearchID)
{
	return ((TESForm_CK *(__fastcall *)(uint32_t))OFFSET(0x16B8780, 1530))(SearchID);
}

void *TESForm_CK::AlteredFormList_Create(BSTArray<TESForm_CK *> *Array, uint32_t Unknown)
{
	AlteredFormListShadow.clear();

	return ((void *(__fastcall *)(BSTArray<TESForm_CK *> *, uint32_t))OFFSET(0x16C6990, 1530))(Array, Unknown);
}

void TESForm_CK::AlteredFormList_RemoveAllEntries(BSTArray<TESForm_CK *> *Array, bool Unknown)
{
	AlteredFormListShadow.clear();

	((void(__fastcall *)(BSTArray<TESForm_CK *> *, bool))OFFSET(0x139B2B0, 1530))(Array, Unknown);
}

void TESForm_CK::AlteredFormList_Insert(BSTArray<TESForm_CK *> *Array, TESForm_CK *&Entry)
{
	AlteredFormListShadow.insert(Entry);

	((void(__fastcall *)(BSTArray<TESForm_CK *> *, TESForm_CK *&))OFFSET(0x146A660, 1530))(Array, Entry);
}

void TESForm_CK::AlteredFormList_RemoveEntry(BSTArray<TESForm_CK *> *Array, uint32_t Index, uint32_t Unknown)
{
	AlteredFormListShadow.erase(Array->at(Index));

	((void(__fastcall *)(BSTArray<TESForm_CK *> *, uint32_t, uint32_t))OFFSET(0x165EA50, 1530))(Array, Index, Unknown);
}

bool TESForm_CK::AlteredFormList_ElementExists(BSTArray<TESForm_CK *> *Array, TESForm_CK *&Entry)
{
	return AlteredFormListShadow.count(Entry) > 0;
}

void FormReferenceMap_RemoveAllEntries()
{
	for (auto [k, v] : FormReferenceMap)
	{
		if (v)
			((void(__fastcall *)(void *, int))OFFSET(0x149F560, 1530))(v, 1);
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
		void *ptr = ((void *(__fastcall *)(size_t))OFFSET(0x1219450, 1530))(24);

		if (ptr)
			ptr = ((void *(__fastcall *)(void *))OFFSET(0x1397CD0, 1530))(ptr);

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

		((void(__fastcall *)(void *, int))OFFSET(0x149F560, 1530))(ptr, 1);
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