#pragma once

#include "../TES/BSTArray.h"
#include "BSHandleRefObject_CK.h"

class TESForm_CK
{
public:
	virtual ~TESForm_CK();
	char _pad0[0x8];
	uint32_t FormFlags;
	uint32_t FormID;
	char _pad1[0x10];

	bool GetActive() const;
	uint32_t GetFormID() const;

	static TESForm_CK *GetFormByNumericID(uint32_t SearchID);
	static void *AlteredFormList_Create(BSTArray<TESForm_CK *> *Array, uint32_t Unknown);
	static void AlteredFormList_RemoveAllEntries(BSTArray<TESForm_CK *> *Array, bool Unknown);
	static void AlteredFormList_Insert(BSTArray<TESForm_CK *> *Array, TESForm_CK *&Entry);
	static void AlteredFormList_RemoveEntry(BSTArray<TESForm_CK *> *Array, uint32_t Index, uint32_t Unknown);
	static bool AlteredFormList_ElementExists(BSTArray<TESForm_CK *> *Array, TESForm_CK *&Entry);
};
static_assert(sizeof(TESForm_CK) == 0x28);
static_assert_offset(TESForm_CK, FormFlags, 0x10);
static_assert_offset(TESForm_CK, FormID, 0x14);

class TESChildCell_CK
{
public:
	virtual ~TESChildCell_CK();
	virtual void *GetSaveParentCell();
};
static_assert(sizeof(TESChildCell_CK) == 0x8);

class TESObjectREFR_CK : public TESForm_CK, public TESChildCell_CK, public BSHandleRefObject
{
public:
	virtual ~TESObjectREFR_CK();
	virtual void OtherTestFunction2();
	char _pad0[0x5C];
};
static_assert(sizeof(TESObjectREFR_CK) == 0xA0);

void FormReferenceMap_RemoveAllEntries();
void *FormReferenceMap_FindOrCreate(uint64_t Key, bool Create);
void FormReferenceMap_RemoveEntry(uint64_t Key);
bool FormReferenceMap_Get(uint64_t Unused, uint64_t Key, void **Value);