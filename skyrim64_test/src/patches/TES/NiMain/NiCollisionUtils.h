#pragma once

#include "NiPoint.h"

class NiCollisionUtils
{
public:
	static bool IntersectTriangle(const NiPoint3& kOrigin, const NiPoint3& kDir, const NiPoint3& kV1, const NiPoint3& kV2, const NiPoint3& kV3, bool bCull, NiPoint3& kIntersect, float& r, float& s, float& t);
};