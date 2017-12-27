#include "../../../common.h"
#include "BSShaderUtil.h"

DirectX::XMMATRIX BSShaderUtil::GetXMFromNi(const NiTransform& Transform)
{
	auto sub_1412DA0D0 = (void(__fastcall *)(DirectX::XMMATRIX *, const NiTransform *))(g_ModuleBase + 0x12DA0D0);

	DirectX::XMMATRIX temp;
	sub_1412DA0D0(&temp, &Transform);
	return temp;
}

DirectX::XMMATRIX BSShaderUtil::GetXMFromNiPosAdjust(const NiTransform& Transform, const NiPoint3& PosAdjust)
{
	DirectX::XMMATRIX temp = GetXMFromNi(Transform);
	float *vals = (float *)&temp;

	vals[12] = Transform.m_Translate.x - PosAdjust.x;
	vals[13] = Transform.m_Translate.y - PosAdjust.y;
	vals[14] = Transform.m_Translate.z - PosAdjust.z;
	vals[15] = 1.0f;

	return temp;
}