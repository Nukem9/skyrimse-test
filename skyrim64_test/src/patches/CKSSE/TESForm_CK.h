#pragma once

#include "../TES/BSTArray.h"
#include "BSHandleRefObject_CK.h"
#include "BSString.h"
#include "BSStringCache.h"

#pragma pack(push, 1)

class TESForm_CK
{
public:
	// Form State
	enum FormFlags : DWORD {
		fsMaster = /*00*/ 0x1,				// form is from an esm file
		fsModified = /*01*/ 0x2,			// form is overriden by active mod or save file
		fsLinked = /*03*/ 0x8,				// set after formids have been resolved into TESForm*
		fsDeleted = /*05*/ 0x20,			// set on deletion, not saved in CK
		fsTemporary = /*0E*/ 0x4000,		// not saved in CK
	};
protected:
	// 06
	struct BSTrackingData {
		WORD date;         // 00 low byte is day of month, high byte is number of months with 1 = Jan. 2003 (Decembers are a little weird)
		WORD pad02;
		BYTE lastUser;     // 04 userID that last had this form checked out
		BYTE currentUser;  // 05 userID that has this form checked out
	};
public:
	virtual ~TESForm_CK();

	using Array = BSTArray<TESForm_CK*>;

	bool GetActive() const;
	bool GetMarkedDelete() const;
	uint32_t GetFormID() const;
	BSString GetEditorID() const;

	void SetNewFormID(uint32_t NewIndex, bool Unk = true);

	static TESForm_CK* GetFormByNumericID(uint32_t SearchID);
	static void* AlteredFormList_Create(Array* Array, uint32_t Unknown);
	static void AlteredFormList_RemoveAllEntries(Array* Array, bool Unknown);
	static void AlteredFormList_Insert(Array* Array, TESForm_CK*& Entry);
	static void AlteredFormList_RemoveEntry(Array* Array, uint32_t Index, uint32_t Unknown);
	static bool AlteredFormList_ElementExists(Array* Array, TESForm_CK*& Entry);
private:
	char _pad0[0x8];
	uint32_t FormFlags;
	uint32_t FormID;
	/*BSStringCache::Ref*/char* EditorID;
	BSTrackingData _trackingData;
	char FormType;
	char _pad1[0x1];
};
static_assert(sizeof(TESForm_CK) == 0x28);

#pragma pack(pop)

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
TESForm_CK::Array *FormReferenceMap_FindOrCreate(uint64_t Key, bool Create);
void FormReferenceMap_RemoveEntry(uint64_t Key);
bool FormReferenceMap_Get(uint64_t Unused, uint64_t Key, TESForm_CK::Array **Value);