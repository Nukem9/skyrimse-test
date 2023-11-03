#include "../../common.h"
#include <unordered_set>
#include "TESForm_CK.h"

static_assert(sizeof(TESForm_CK::Array) == 0x18);

std::unordered_set<TESForm_CK*, std::hash<TESForm_CK*>,
	std::equal_to<TESForm_CK*>, voltek::allocator<TESForm_CK*>> AlteredFormListShadow;
std::unordered_map<uint64_t, TESForm_CK::Array*, std::hash<uint64_t>,
	std::equal_to<uint64_t>, voltek::allocator<std::pair<const uint64_t, 
	TESForm_CK::Array*>>> FormReferenceMap;

bool TESForm_CK::GetActive() const
{
	return (FormFlags & FormFlags::fsModified) != 0;
}

bool TESForm_CK::GetMarkedDelete() const {
	return (FormFlags & FormFlags::fsDeleted) != 0;
}

uint32_t TESForm_CK::GetFormID() const
{
	return FormID;
}

BSString TESForm_CK::GetEditorID() const {
	return thisVirtualCall<char*>(0x1E8, this);
}

void TESForm_CK::SetNewFormID(uint32_t NewIndex, bool Unk) {
	thisVirtualCall<char*>(0x310, this, NewIndex, Unk);
}

TESForm_CK* TESForm_CK::GetFormByNumericID(uint32_t SearchID)
{
	return ((TESForm_CK*(__fastcall*)(uint32_t))OFFSET(0x16B8780, 1530))(SearchID);
}

void *TESForm_CK::AlteredFormList_Create(TESForm_CK::Array *Array, uint32_t Unknown)
{
	AlteredFormListShadow.clear();

	return ((void*(__fastcall*)(TESForm_CK::Array*, uint32_t))OFFSET(0x16C6990, 1530))(Array, Unknown);
}

void TESForm_CK::AlteredFormList_RemoveAllEntries(TESForm_CK::Array *Array, bool Unknown)
{
	AlteredFormListShadow.clear();

	((void(__fastcall*)(TESForm_CK::Array*, bool))OFFSET(0x139B2B0, 1530))(Array, Unknown);
}

void TESForm_CK::AlteredFormList_Insert(TESForm_CK::Array *Array, TESForm_CK *&Entry)
{
	AlteredFormListShadow.insert(Entry);

	((void(__fastcall*)(TESForm_CK::Array*, TESForm_CK*&))OFFSET(0x146A660, 1530))(Array, Entry);
}

void TESForm_CK::AlteredFormList_RemoveEntry(TESForm_CK::Array *Array, uint32_t Index, uint32_t Unknown)
{
	AlteredFormListShadow.erase(Array->at(Index));

	((void(__fastcall*)(TESForm_CK::Array*, uint32_t, uint32_t))OFFSET(0x165EA50, 1530))(Array, Index, Unknown);
}

bool TESForm_CK::AlteredFormList_ElementExists(TESForm_CK::Array *Array, TESForm_CK *&Entry)
{
	return AlteredFormListShadow.count(Entry) > 0;
}

void FormReferenceMap_RemoveAllEntries()
{
	for (auto [k, v] : FormReferenceMap)
	{
		if (v)
			((void(__fastcall*)(TESForm_CK::Array*, int))OFFSET(0x149F560, 1530))(v, 1);
	}

	FormReferenceMap.clear();
}

TESForm_CK::Array *FormReferenceMap_FindOrCreate(uint64_t Key, bool Create)
{
	auto itr = FormReferenceMap.find(Key);

	if (itr != FormReferenceMap.end() && itr->second)
		return itr->second;

	if (Create)
	{
		auto* ptr = ((TESForm_CK::Array*(__fastcall*)(size_t))OFFSET(0x1219450, 1530))(24);

		if (ptr)
			ptr = ((TESForm_CK::Array*(__fastcall*)(TESForm_CK::Array*))OFFSET(0x1397CD0, 1530))(ptr);

		FormReferenceMap.insert_or_assign(Key, ptr);
		return ptr;
	}

	return nullptr;
}

void FormReferenceMap_RemoveEntry(uint64_t Key)
{
	auto itr = FormReferenceMap.find(Key);

	if (itr != FormReferenceMap.end())
	{
		TESForm_CK::Array* ptr = itr->second;
		FormReferenceMap.erase(itr);

		if (ptr)
			((void(__fastcall *)(TESForm_CK::Array*, int))OFFSET(0x149F560, 1530))(ptr, 1);
	}
}

bool FormReferenceMap_Get(uint64_t Unused, uint64_t Key, TESForm_CK::Array **Value)
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