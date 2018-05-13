#pragma once

#include <DirectXMath.h>
#include "../NiMain/NiTransform.h"

class BSShaderUtil
{
public:
	static DirectX::XMMATRIX GetXMFromNi(const NiTransform& Transform);
	static DirectX::XMMATRIX GetXMFromNiPosAdjust(const NiTransform& Transform, const NiPoint3& PosAdjust);
	static void TransposeStoreMatrix3x4(float *Dest, const DirectX::XMMATRIX& Source);
	static void StoreTransform3x4NoScale(float Dest[3][4], const NiTransform& Source);
};