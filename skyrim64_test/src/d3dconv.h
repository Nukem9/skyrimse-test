#pragma once

#include <DirectXMath.h>

#define D3D_EXPORT	extern "C" __declspec(dllexport)
#define D3DAPI		__stdcall

typedef struct _D3DMATRIX
{
	union
	{
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};

		float m[4][4];
	};
} D3DMATRIX;

typedef struct _D3DXMATRIX : public D3DMATRIX
{
} D3DXMATRIX;

typedef struct _D3DXPLANE
{
	float a;
	float b;
	float c;
	float d;
} D3DXPLANE, *LPD3DXPLANE;

typedef struct _D3DXVECTOR3
{
	float x;
	float y;
	float z;
} D3DXVECTOR3, *LPD3DXVECTOR3;