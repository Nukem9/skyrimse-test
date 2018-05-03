#pragma once

#include "BSTArray.h"
#include "TESForm.h"

class BGSDistantTreeBlock/* : public BSResourceEntry */
{
public:
	struct LODGroupInstance
	{
		uint32_t FormId;	// Only the lower 24 bits used
		char _pad[0xA];
		uint16_t Alpha;		// This is Float2Word(fAlpha)
		bool Hidden;		// Alpha <= 0.0f or set by object flags
	};

	struct LODGroup
	{
		char _pad[8];
		BSTArray<LODGroupInstance> m_LODInstances;
		char _pad2[4];
		bool m_UnkByte24;
	};

	struct ResourceData
	{
		BSTArray<LODGroup *> m_LODGroups;
		char _pad[106];
		bool m_UnkByte82;
	};

	static void InvalidateCachedForm(uint32_t FormId);
	static void UpdateLODAlphaFade(ResourceData *Data);
	// struct ResourceData @ 0x28
};
static_assert(sizeof(BGSDistantTreeBlock::LODGroupInstance) == 0x14);
static_assert_offset(BGSDistantTreeBlock::LODGroupInstance, FormId, 0x0);
static_assert_offset(BGSDistantTreeBlock::LODGroupInstance, Alpha, 0xE);
static_assert_offset(BGSDistantTreeBlock::LODGroupInstance, Hidden, 0x10);

static_assert_offset(BGSDistantTreeBlock::LODGroup, m_LODInstances, 0x8);
static_assert_offset(BGSDistantTreeBlock::LODGroup, m_UnkByte24, 0x24);

static_assert_offset(BGSDistantTreeBlock::ResourceData, m_LODGroups, 0x0);
static_assert_offset(BGSDistantTreeBlock::ResourceData, m_UnkByte82, 0x82);