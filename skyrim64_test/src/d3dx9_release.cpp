#include "d3dconv.h"
#include "xutil.h"

#ifndef _DEBUG

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
#ifdef __AVX2__
	const auto& M1 = *pM1;
	const auto& M2 = *pM2;
	auto& mResult = *pOut;

	// Splat the component X,Y,Z then W
	DirectX::XMVECTOR vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 0);
	DirectX::XMVECTOR vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 1);
	DirectX::XMVECTOR vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 2);
	DirectX::XMVECTOR vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 3);

	// Perform the operation on the first row
	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);

	// Perform a binary add to reduce cumulative errors
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	mResult.r[0] = vX;

	// Repeat for the other 3 rows
	vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 0);
	vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 1);
	vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 2);
	vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 3);

	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	mResult.r[1] = vX;

	vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 0);
	vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 1);
	vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 2);
	vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 3);

	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	mResult.r[2] = vX;

	vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 0);
	vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 1);
	vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 2);
	vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 3);

	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	mResult.r[3] = vX;
#else
	*pOut = DirectX::XMMatrixMultiply(*pM1, *pM2);
#endif
}

D3D_EXPORT
void D3DAPI D3DXMatrixMultiplyTranspose(
	_Inout_       DirectX::XMMATRIX *pOut,
	_In_    const DirectX::XMMATRIX *pM1,
	_In_    const DirectX::XMMATRIX *pM2
)
{
#ifdef __AVX2__
	const auto& M1 = *pM1;
	const auto& M2 = *pM2;
	auto& mResult = *pOut;

	DirectX::XMVECTOR vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 0);
	DirectX::XMVECTOR vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 1);
	DirectX::XMVECTOR vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 2);
	DirectX::XMVECTOR vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 3);

	// Perform the operation on the first row
	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);

	// Perform a binary add to reduce cumulative errors
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	DirectX::XMVECTOR r0 = vX;

	// Repeat for the other 3 rows
	vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 0);
	vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 1);
	vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 2);
	vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 3);

	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	DirectX::XMVECTOR r1 = vX;

	vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 0);
	vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 1);
	vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 2);
	vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 3);

	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	DirectX::XMVECTOR r2 = vX;

	vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 0);
	vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 1);
	vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 2);
	vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 3);

	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	DirectX::XMVECTOR r3 = vX;

	// x.x,x.y,y.x,y.y
	DirectX::XMVECTOR vTemp1 = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(1, 0, 1, 0));
	// x.z,x.w,y.z,y.w
	DirectX::XMVECTOR vTemp3 = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(3, 2, 3, 2));
	// z.x,z.y,w.x,w.y
	DirectX::XMVECTOR vTemp2 = _mm_shuffle_ps(r2, r3, _MM_SHUFFLE(1, 0, 1, 0));
	// z.z,z.w,w.z,w.w
	DirectX::XMVECTOR vTemp4 = _mm_shuffle_ps(r2, r3, _MM_SHUFFLE(3, 2, 3, 2));

	// x.x,y.x,z.x,w.x
	mResult.r[0] = _mm_shuffle_ps(vTemp1, vTemp2, _MM_SHUFFLE(2, 0, 2, 0));
	// x.y,y.y,z.y,w.y
	mResult.r[1] = _mm_shuffle_ps(vTemp1, vTemp2, _MM_SHUFFLE(3, 1, 3, 1));
	// x.z,y.z,z.z,w.z
	mResult.r[2] = _mm_shuffle_ps(vTemp3, vTemp4, _MM_SHUFFLE(2, 0, 2, 0));
	// x.w,y.w,z.w,w.w
	mResult.r[3] = _mm_shuffle_ps(vTemp3, vTemp4, _MM_SHUFFLE(3, 1, 3, 1));
#else
	*pOut = DirectX::XMMatrixMultiplyTranspose(*pM1, *pM2);
#endif
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

STATIC_CONSTRUCTOR(__D3DX9Validate,
[]{
	static_assert(sizeof(D3DXMATRIX) == sizeof(DirectX::XMMATRIX), "Matrix size");
	static_assert(sizeof(D3DXPLANE) == sizeof(DirectX::XMVECTOR), "Plane size");
});

#endif // ndef _DEBUG
