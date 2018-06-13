#include "d3dconv.h"
#include "xutil.h"

#ifdef _DEBUG

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

//#define TEST_MATRIX(x) (x)
#define TEST_MATRIX(x) ((D3DXMATRIX *)0x5550000DEADBEEF)

//#define TEST_PLANE(x) (x)
#define TEST_PLANE(x) ((D3DXPLANE *)0x5550000DEADBEEF)

#define AS_XMM(x)		(*(DirectX::XMMATRIX *)x)
#define AS_VECTOR(x)	(*(DirectX::XMVECTOR *)x)

D3D_EXPORT
D3DXMATRIX* D3DAPI D3DXMatrixInverse(
	_Inout_       D3DXMATRIX *pOut,
	_Inout_       FLOAT      *pDeterminant,
	_In_    const D3DXMATRIX *pM
)
{
	// Ignored in engine
	assert(!pDeterminant);

	AS_XMM(pOut) = DirectX::XMMatrixInverse(nullptr, AS_XMM(pM));
	return TEST_MATRIX(pOut);
}

D3D_EXPORT
D3DXMATRIX* D3DAPI D3DXMatrixTranslation(
	_Inout_ DirectX::XMMATRIX	*pOut,
	_In_    FLOAT				x,
	_In_    FLOAT				y,
	_In_    FLOAT				z
)
{
	AS_XMM(pOut) = DirectX::XMMatrixTranslation(x, y, z);
	return TEST_MATRIX(pOut);
}

D3D_EXPORT
D3DXMATRIX* D3DAPI D3DXMatrixMultiply(
	_Inout_       D3DXMATRIX *pOut,
	_In_    const D3DXMATRIX *pM1,
	_In_    const D3DXMATRIX *pM2
)
{
	AS_XMM(pOut) = DirectX::XMMatrixMultiply(AS_XMM(pM1), AS_XMM(pM2));
	return TEST_MATRIX(pOut);
}

D3D_EXPORT
D3DXMATRIX* D3DAPI D3DXMatrixMultiplyTranspose(
	_Inout_       D3DXMATRIX *pOut,
	_In_    const D3DXMATRIX *pM1,
	_In_    const D3DXMATRIX *pM2
)
{
	AS_XMM(pOut) = DirectX::XMMatrixMultiplyTranspose(AS_XMM(pM1), AS_XMM(pM2));
	return TEST_MATRIX(pOut);
}

D3D_EXPORT
D3DXMATRIX* D3DAPI D3DXMatrixTranspose(
	_Inout_       D3DXMATRIX *pOut,
	_In_    const D3DXMATRIX *pM
)
{
	AS_XMM(pOut) = DirectX::XMMatrixTranspose(AS_XMM(pM));
	return TEST_MATRIX(pOut);
}

D3D_EXPORT
D3DXPLANE* D3DAPI D3DXPlaneNormalize(
	_Inout_       D3DXPLANE *pOut,
	_In_    const D3DXPLANE *pP
)
{
	AS_VECTOR(pOut) = DirectX::XMPlaneNormalize(AS_VECTOR(pP));
	return TEST_PLANE(pOut);
}

D3D_EXPORT
D3DXPLANE* D3DAPI D3DXPlaneTransform(
	_Inout_       D3DXPLANE  *pOut,
	_In_    const D3DXPLANE  *pP,
	_In_    const D3DXMATRIX *pM
)
{
	AS_VECTOR(pOut) = DirectX::XMPlaneTransform(AS_VECTOR(pP), AS_XMM(pM));
	return TEST_PLANE(pOut);
}

D3D_EXPORT
D3DXVECTOR3* D3DAPI D3DXVec3Normalize(
	_Inout_       D3DXVECTOR3 *pOut,
	_In_    const D3DXVECTOR3 *pV
)
{
	DirectX::XMVECTORF32 v;
	DirectX::XMVECTORF32 o;
	v.f[0] = pV->x;
	v.f[1] = pV->y;
	v.f[2] = pV->z;

	o.v = DirectX::XMVector3Normalize(v);

	pOut->x = o.f[0];
	pOut->y = o.f[1];
	pOut->z = o.f[2];
	return pOut;
}

D3D_EXPORT
D3DXVECTOR3* D3DAPI D3DXVec3TransformCoord(
	_Inout_       D3DXVECTOR3 *pOut,
	_In_    const D3DXVECTOR3 *pV,
	_In_    const D3DXMATRIX  *pM
)
{
	DirectX::XMVECTORF32 v;
	DirectX::XMVECTORF32 o;
	v.f[0] = pV->x;
	v.f[1] = pV->y;
	v.f[2] = pV->z;

	o.v = DirectX::XMVector3TransformCoord(v, AS_XMM(pM));

	pOut->x = o.f[0];
	pOut->y = o.f[1];
	pOut->z = o.f[2];
	return pOut;
}

D3D_EXPORT
D3DXVECTOR3* D3DAPI D3DXVec3TransformNormal(
	_Inout_       D3DXVECTOR3 *pOut,
	_In_    const D3DXVECTOR3 *pV,
	_In_    const D3DXMATRIX  *pM
)
{
	DirectX::XMVECTORF32 v;
	DirectX::XMVECTORF32 o;
	v.f[0] = pV->x;
	v.f[1] = pV->y;
	v.f[2] = pV->z;

	o.v = DirectX::XMVector3TransformNormal(v, AS_XMM(pM));

	pOut->x = o.f[0];
	pOut->y = o.f[1];
	pOut->z = o.f[2];
	return pOut;
}

STATIC_CONSTRUCTOR(__D3DX9Validate,
[]{
	// Matrices
	static_assert(sizeof(D3DXMATRIX) == sizeof(DirectX::XMMATRIX), "Matrix size");

	D3DXMATRIX *matrix = nullptr;
	DirectX::XMMATRIX *xmMatrix = nullptr;

	if ((size_t)&matrix->m[0][0] != (size_t)&xmMatrix->r[0])
		assert(false && "Matrix 0");

	if ((size_t)&matrix->m[1][0] != (size_t)&xmMatrix->r[1])
		assert(false && "Matrix 1");

	if ((size_t)&matrix->m[2][0] != (size_t)&xmMatrix->r[2])
		assert(false && "Matrix 2");

	if ((size_t)&matrix->m[3][0] != (size_t)&xmMatrix->r[3])
		assert(false && "Matrix 3");

	// Planes (Vector4)
	static_assert(sizeof(D3DXPLANE) == sizeof(DirectX::XMVECTOR), "Plane size");

	D3DXPLANE *plane = nullptr;
	DirectX::XMVECTOR *xmPlane = nullptr;

	if ((size_t)&plane->a != (size_t)&xmPlane->m128_f32[0])
		assert(false && "Plane 0");

	if ((size_t)&plane->b != (size_t)&xmPlane->m128_f32[1])
		assert(false && "Plane 1");

	if ((size_t)&plane->c != (size_t)&xmPlane->m128_f32[2])
		assert(false && "Plane 2");

	if ((size_t)&plane->d != (size_t)&xmPlane->m128_f32[3])
		assert(false && "Plane 3");
});

#endif // _DEBUG