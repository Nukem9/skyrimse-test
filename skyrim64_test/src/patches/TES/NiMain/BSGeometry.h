#pragma once

//
// Yes, according to NiMain.dll, this file should be in NiMain
//
#include "NiAVObject.h"
#include "NiAlphaProperty.h"
#include "NiSkinInstance.h"

class BSGeometry : public NiAVObject
{
public:
	virtual ~BSGeometry();

	char _pad[0x48];

	NiAlphaProperty *QAlphaProperty() const
	{
		return *(NiAlphaProperty **)((uintptr_t)this + 0x120);
	}

	NiSkinInstance *QSkinInstance() const
	{
		return *(NiSkinInstance **)((uintptr_t)this + 0x130);
	}

	int QType() const
	{
		return *(uint8_t *)((uintptr_t)this + 0x150);
	}
};
static_assert(sizeof(BSGeometry) == 0x158);