#pragma once

#include "../../common.h"
#include "../TES/BSTArray.h"

class BSTriShape;
class TESForm;

class TESWaterObject
{
public:
	char _pad0[0x20];
	BSTriShape *m_TriShape;
	char _pad1[0x8];
	TESForm *m_BaseWaterForm;
};
static_assert_offset(TESWaterObject, m_TriShape, 0x20);
static_assert_offset(TESWaterObject, m_BaseWaterForm, 0x30);

class TESWaterRoot
{
public:
	char _pad0[0x28];
	BSTArray<TESWaterObject *> m_WaterObjects;

	static TESWaterRoot *Singleton()
	{
		return ((TESWaterRoot *(__fastcall *)())(g_ModuleBase + 0x130E050))();
	}
};
static_assert_offset(TESWaterRoot, m_WaterObjects, 0x28);