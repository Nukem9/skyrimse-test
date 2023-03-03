#pragma once

#include "../TES/NiMain/NiPoint.h"
#include "../TES/NiMain/NiPick.h"

class IRendererResourceManager_CK
{
};

class BSShaderResourceManager_CK : public IRendererResourceManager_CK
{
public:
	bool FindIntersectionsTriShapeFastPath(NiPoint3& kOrigin, const NiPoint3& kDir, NiPick& kPick, class BSTriShape *pkTriShape);
	bool FindIntersectionsTriShapeFastPathEx(NiPoint3& kOrigin, const NiPoint3& kDir, NiPick& kPick, class BSTriShape* pkTriShape);
};