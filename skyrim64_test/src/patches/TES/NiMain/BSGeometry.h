#pragma once

//
// Yes, according to NiMain.dll, this file should be in NiMain
//
#include "NiAVObject.h"

class BSGeometry : public NiAVObject
{
	virtual ~BSGeometry();

	char _pad[0x48];
};
static_assert(sizeof(BSGeometry) == 0x158);