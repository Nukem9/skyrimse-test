#pragma once

#include <Windows.h>
#include <DirectXMath.h>

#define D3D_EXPORT	extern "C" __declspec(dllexport)
#define D3DAPI		WINAPI

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
	FLOAT a;
	FLOAT b;
	FLOAT c;
	FLOAT d;
} D3DXPLANE, *LPD3DXPLANE;

typedef struct _D3DXVECTOR3
{
	FLOAT x;
	FLOAT y;
	FLOAT z;
} D3DXVECTOR3, *LPD3DXVECTOR3;