#include "../../../common.h"
#include "../BSGraphics/BSGraphicsRenderer.h"
#include "BSShaderUtil.h"

using namespace DirectX;

XMMATRIX BSShaderUtil::GetXMFromNi(const NiTransform& Transform)
{
	return GetXMFromNiPosAdjust(Transform, BSGraphics::Renderer::QInstance()->GetRendererShadowState()->m_PosAdjust);
}

XMMATRIX BSShaderUtil::GetXMFromNiPosAdjust(const NiTransform& Transform, const NiPoint3& PosAdjust)
{
	XMMATRIX temp;

	const NiMatrix3& m = Transform.m_Rotate;
	const float scale = Transform.m_fScale;

	temp.r[0] = XMVectorScale(XMVectorSet(
		m.m_pEntry[0][0],
		m.m_pEntry[1][0],
		m.m_pEntry[2][0],
		0.0f), scale);

	temp.r[1] = XMVectorScale(XMVectorSet(
		m.m_pEntry[0][1],
		m.m_pEntry[1][1],
		m.m_pEntry[2][1],
		0.0f), scale);

	temp.r[2] = XMVectorScale(XMVectorSet(
		m.m_pEntry[0][2],
		m.m_pEntry[1][2],
		m.m_pEntry[2][2],
		0.0f), scale);

	temp.r[3] = XMVectorSet(
		Transform.m_Translate.x - PosAdjust.x,
		Transform.m_Translate.y - PosAdjust.y,
		Transform.m_Translate.z - PosAdjust.z,
		1.0f);

	return temp;
}

void BSShaderUtil::TransposeStoreMatrix3x4(float *Dest, const XMMATRIX& Source)
{
	XMMATRIX transposed = XMMatrixTranspose(Source);

	_mm_store_ps(&Dest[0], transposed.r[0]);
	_mm_store_ps(&Dest[4], transposed.r[1]);
	_mm_store_ps(&Dest[8], transposed.r[2]);
	// Implied Dest[12...15] = { 0, 0, 0, 1 };
}

void BSShaderUtil::StoreTransform3x4NoScale(float Dest[3][4], const NiTransform& Source)
{
	//
	// Shove a Matrix3+Point3 directly into a float[3][4] with no modifications
	//
	// Dest[0][#] = Source.m_Rotate.m_pEntry[0][#];
	// Dest[0][3] = Source.m_Translate.x;
	// Dest[1][#] = Source.m_Rotate.m_pEntry[1][#];
	// Dest[1][3] = Source.m_Translate.x;
	// Dest[2][#] = Source.m_Rotate.m_pEntry[2][#];
	// Dest[2][3] = Source.m_Translate.x;
	//
	static_assert(sizeof(NiTransform::m_Rotate) == 3 * 3 * sizeof(float));	// NiMatrix3
	static_assert(sizeof(NiTransform::m_Translate) == 3 * sizeof(float));	// NiPoint3
	static_assert(offsetof(NiTransform, m_Translate) > offsetof(NiTransform, m_Rotate));

	_mm_store_ps(Dest[0], _mm_loadu_ps(Source.m_Rotate.m_pEntry[0]));
	_mm_store_ps(Dest[1], _mm_loadu_ps(Source.m_Rotate.m_pEntry[1]));
	_mm_store_ps(Dest[2], _mm_loadu_ps(Source.m_Rotate.m_pEntry[2]));

	Dest[0][3] = Source.m_Translate.x;
	Dest[1][3] = Source.m_Translate.y;
	Dest[2][3] = Source.m_Translate.z;
}