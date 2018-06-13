#include "d3dconv.h"
#include "xutil.h"

#ifndef _DEBUG

#pragma comment(linker, "/export:D3DXSaveSurfaceToFileA=C:\\Windows\\System32\\d3dx9_42.D3DXSaveSurfaceToFileA")
#pragma comment(linker, "/export:D3DXSaveTextureToFileA=C:\\Windows\\System32\\d3dx9_42.D3DXSaveTextureToFileA")
#pragma comment(linker, "/export:D3DXCreateTexture=C:\\Windows\\System32\\d3dx9_42.D3DXCreateTexture")
#pragma comment(linker, "/export:D3DXCreateTextureFromFileA=C:\\Windows\\System32\\d3dx9_42.D3DXCreateTextureFromFileA")
#pragma comment(linker, "/export:D3DXCreateTextureFromFileExA=C:\\Windows\\System32\\d3dx9_42.D3DXCreateTextureFromFileExA")
#pragma comment(linker, "/export:D3DXGetImageInfoFromFileA=C:\\Windows\\System32\\d3dx9_42.D3DXGetImageInfoFromFileA")
#pragma comment(linker, "/export:D3DXMatrixRotationQuaternion=C:\\Windows\\System32\\d3dx9_42.D3DXMatrixRotationQuaternion")
#pragma comment(linker, "/export:D3DXMatrixOrthoLH=C:\\Windows\\System32\\d3dx9_42.D3DXMatrixOrthoLH")
#pragma comment(linker, "/export:D3DXMatrixPerspectiveFovLH=C:\\Windows\\System32\\d3dx9_42.D3DXMatrixPerspectiveFovLH")
#pragma comment(linker, "/export:D3DXMatrixLookAtLH=C:\\Windows\\System32\\d3dx9_42.D3DXMatrixLookAtLH")
#pragma comment(linker, "/export:D3DXQuaternionMultiply=C:\\Windows\\System32\\d3dx9_42.D3DXQuaternionMultiply")

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
void D3DAPI D3DXMatrixTranslation(
	_Inout_ DirectX::XMMATRIX	*pOut,
	_In_    FLOAT				x,
	_In_    FLOAT				y,
	_In_    FLOAT				z
)
{
	*pOut = DirectX::XMMatrixTranslation(x, y, z);
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

STATIC_CONSTRUCTOR(__D3DX9Validate,
[]{
	static_assert(sizeof(D3DXMATRIX) == sizeof(DirectX::XMMATRIX), "Matrix size");
	static_assert(sizeof(D3DXPLANE) == sizeof(DirectX::XMVECTOR), "Plane size");
});

#endif // ndef _DEBUG
