#include "stdafx.h"

#ifndef _DEBUG
#pragma check_stack(off)
#pragma runtime_checks("", off)

D3D_EXPORT
void D3DAPI D3DXMatrixInverse(
	_Inout_       DirectX::XMMATRIX *pOut,
	_Inout_       FLOAT				*pDeterminant,
	_In_    const DirectX::XMMATRIX *pM
)
{
	*pOut = DirectX::XMMatrixInverse(nullptr, *pM);
}

D3D_EXPORT
void D3DAPI D3DXMatrixMultiply(
	_Inout_       DirectX::XMMATRIX *pOut,
	_In_    const DirectX::XMMATRIX *pM1,
	_In_    const DirectX::XMMATRIX *pM2
)
{
	*pOut = DirectX::XMMatrixMultiply(*pM1, *pM2);
}

D3D_EXPORT
void D3DAPI D3DXMatrixMultiplyTranspose(
	_Inout_       DirectX::XMMATRIX *pOut,
	_In_    const DirectX::XMMATRIX *pM1,
	_In_    const DirectX::XMMATRIX *pM2
)
{
	*pOut = DirectX::XMMatrixMultiplyTranspose(*pM1, *pM2);
}

D3D_EXPORT
void D3DAPI D3DXMatrixTranspose(
	_Inout_       DirectX::XMMATRIX *pOut,
	_In_    const DirectX::XMMATRIX *pM
)
{
	*pOut = DirectX::XMMatrixTranspose(*pM);
}

D3D_EXPORT
void D3DAPI D3DXPlaneNormalize(
	_Inout_       DirectX::XMVECTOR *pOut,
	_In_    const DirectX::XMVECTOR *pP
)
{
	*pOut = DirectX::XMPlaneNormalize(*pP);
}

D3D_EXPORT
void D3DAPI D3DXPlaneTransform(
	_Inout_       DirectX::XMVECTOR *pOut,
	_In_    const DirectX::XMVECTOR *pP,
	_In_    const DirectX::XMMATRIX *pM
)
{
	*pOut = DirectX::XMPlaneTransform(*pP, *pM);
}

D3D_EXPORT
D3DXVECTOR3* D3DAPI D3DXVec3Normalize(
	_Inout_       D3DXVECTOR3 *pOut,
	_In_    const D3DXVECTOR3 *pV
)
{
	DirectX::XMVECTORF32 v;
	v.v = DirectX::XMVector3Normalize(_mm_setr_ps(pV->x, pV->y, pV->z, 0.0f));

	pOut->x = v.f[0];
	pOut->y = v.f[1];
	pOut->z = v.f[2];
	return pOut;
}

D3D_EXPORT
D3DXVECTOR3* D3DAPI D3DXVec3TransformCoord(
	_Inout_       D3DXVECTOR3		*pOut,
	_In_    const D3DXVECTOR3		*pV,
	_In_    const DirectX::XMMATRIX	*pM
)
{
	DirectX::XMVECTORF32 v;
	v.v = DirectX::XMVector3TransformCoord(_mm_setr_ps(pV->x, pV->y, pV->z, 0.0f), *pM);

	pOut->x = v.f[0];
	pOut->y = v.f[1];
	pOut->z = v.f[2];
	return pOut;
}

D3D_EXPORT
D3DXVECTOR3* D3DAPI D3DXVec3TransformNormal(
	_Inout_       D3DXVECTOR3		*pOut,
	_In_    const D3DXVECTOR3		*pV,
	_In_    const DirectX::XMMATRIX	*pM
)
{
	DirectX::XMVECTORF32 v;
	v.v = DirectX::XMVector3TransformNormal(_mm_setr_ps(pV->x, pV->y, pV->z, 0.0f), *pM);

	pOut->x = v.f[0];
	pOut->y = v.f[1];
	pOut->z = v.f[2];
	return pOut;
}

void D3DXValidateStructures()
{
	static_assert(sizeof(D3DXMATRIX) == sizeof(DirectX::XMMATRIX), "Matrix size");
	static_assert(sizeof(D3DXPLANE) == sizeof(DirectX::XMVECTOR), "Plane size");
}

#pragma runtime_checks("", restore)
#pragma check_stack()
#endif // ndef _DEBUG