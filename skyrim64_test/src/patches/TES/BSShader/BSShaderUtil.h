#pragma once

#include <DirectXMath.h>
#include "../NiMain/NiTransform.h"

class BSShaderUtil
{
public:
	static DirectX::XMMATRIX GetXMFromNi(const NiTransform& Transform);
	static DirectX::XMMATRIX GetXMFromNiPosAdjust(const NiTransform& Transform, const NiPoint3& PosAdjust);
};