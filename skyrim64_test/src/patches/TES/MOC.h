#pragma once

#include <DirectXMath.h>

class MaskedOcclusionCulling;

namespace MOC
{
	void Init();
	void RegisterGeometry(BSGeometry *Geometry);
	void SendTraverseCommand(NiCamera *Camera);
	void TraverseSceneGraph(NiCamera *Camera);
	void RemoveCachedVerticesAndIndices(void *RendererData);
	void UpdateDepthViewTexture();
	void ForceFlush();

	void RenderGeometryCallback(MaskedOcclusionCulling *MOC, void *UserData);
	void TraverseSceneGraphCallback(MaskedOcclusionCulling *MOC, void *UserData);

	bool TestObject(NiAVObject *Object);

	using namespace DirectX;

	struct fplanes
	{
		alignas(64) XMVECTOR	Planes[6];
		bool					Side[6][4];
		alignas(64) __m128		PlaneComponents[8];

		void CreateFromViewProjMatrix(XMMATRIX M)
		{
			M = XMMatrixTranspose(M);
			XMVECTOR col1 = M.r[0];// { _11, _21, _31, _41 }
			XMVECTOR col2 = M.r[1];// { _12, _22, _32, _42 }
			XMVECTOR col3 = M.r[2];// { _13, _23, _33, _43 }
			XMVECTOR col4 = M.r[3];// { _14, _24, _34, _44 }

			Planes[0] = XMPlaneNormalize(XMVectorAdd(col4, col1));		// [c4 + c1] Left
			Planes[1] = XMPlaneNormalize(XMVectorSubtract(col4, col1));	// [c4 - c1] Right
			Planes[2] = XMPlaneNormalize(XMVectorSubtract(col4, col2));	// [c4 - c2] Top
			Planes[3] = XMPlaneNormalize(XMVectorAdd(col4, col2));		// [c4 + c2] Bottom
			Planes[4] = XMPlaneNormalize(XMVectorAdd(col4, col3));		// [c4 + c3] Near
			Planes[5] = XMPlaneNormalize(XMVectorSubtract(col4, col3));	// [c4 - c3] Far

			// AABB signed-ness
			for (int i = 0; i < 6; i++)
			{
				Side[i][0] = Planes[i].m128_f32[0] > 0.0f;
				Side[i][1] = Planes[i].m128_f32[1] > 0.0f;
				Side[i][2] = Planes[i].m128_f32[2] > 0.0f;
				Side[i][3] = Planes[i].m128_f32[3] > 0.0f;
			}

			// Sphere test optimization
			auto planes = (float(*)[4])&Planes;

			PlaneComponents[0] = _mm_setr_ps(-planes[0][0], -planes[1][0], -planes[2][0], -planes[3][0]);
			PlaneComponents[1] = _mm_setr_ps(-planes[0][1], -planes[1][1], -planes[2][1], -planes[3][1]);
			PlaneComponents[2] = _mm_setr_ps(-planes[0][2], -planes[1][2], -planes[2][2], -planes[3][2]);
			PlaneComponents[3] = _mm_setr_ps(-planes[0][3], -planes[1][3], -planes[2][3], -planes[3][3]);
			PlaneComponents[4] = _mm_setr_ps(-planes[4][0], -planes[5][0], -planes[4][0], -planes[5][0]);
			PlaneComponents[5] = _mm_setr_ps(-planes[4][1], -planes[5][1], -planes[4][1], -planes[5][1]);
			PlaneComponents[6] = _mm_setr_ps(-planes[4][2], -planes[5][2], -planes[4][2], -planes[5][2]);
			PlaneComponents[7] = _mm_setr_ps(-planes[4][3], -planes[5][3], -planes[4][3], -planes[5][3]);
		}

		bool TestSphere(float Center[3], float Radius)
		{
			return TestSphere(_mm_setr_ps(Center[0], Center[1], Center[2], Radius));
		}

		//
		// https://github.com/nsf/sseculling/blob/master/Common.cpp#L71 by "nsf"
		//
		bool TestSphere(XMVECTOR Center_X_Y_Z_Radius)
		{
			// we negate everything because we use this formula to cull:
			//   dot(-p.n, s.center) - p.d > s.radius
			// it's equivalent to:
			//   dot(p.n, s.center) + p.d < -s.radius
			// but no need to negate sphere radius
			// const __m128 PlaneComponents[8] = { ... };

			// Load sphere into SSE register.
			const __m128 xxxx = XMVectorSplatX(Center_X_Y_Z_Radius);
			const __m128 yyyy = XMVectorSplatY(Center_X_Y_Z_Radius);
			const __m128 zzzz = XMVectorSplatZ(Center_X_Y_Z_Radius);
			const __m128 rrrr = XMVectorSplatW(Center_X_Y_Z_Radius);

			__m128 v;
			__m128 r;

			// Move sphere center to plane normal space and make it relative to plane.
			// dot(p.n, s) + p.d
			v = simd_madd(xxxx, PlaneComponents[0], PlaneComponents[3]);
			v = simd_madd(yyyy, PlaneComponents[1], v);
			v = simd_madd(zzzz, PlaneComponents[2], v);

			// One of r floats will be set to 0xFFFFFFFF if sphere is outside of the frustum.
			r = _mm_cmpgt_ps(v, rrrr);

			// Same for second set of planes.
			v = simd_madd(xxxx, PlaneComponents[4], PlaneComponents[7]);
			v = simd_madd(yyyy, PlaneComponents[5], v);
			v = simd_madd(zzzz, PlaneComponents[6], v);

			r = _mm_or_ps(r, _mm_cmpgt_ps(v, rrrr));

			// Shuffle and extract the result:
			// 1. movehl(r, r) does this (we're interested in 2 lower floats):
			//    a b c d -> c d c d
			// 2. then we OR it with the existing value (ignoring 2 upper floats)
			//    a b | c d = A B
			// 3. and then we OR it again ignoring all but 1 lowest float:
			//    A | B = R
			// Result is written in the lowest float.
			r = _mm_or_ps(r, _mm_movehl_ps(r, r));
			r = _mm_or_ps(r, XMVectorSplatY(r));

			// Culled  = 0xfff...fff
			// Visible = 0x000...000
			return _mm_test_all_ones(*reinterpret_cast<__m128i *>(&r)) != 0;
		}

		bool TestAABB(XMVECTOR Center, XMVECTOR HalfExtents)
		{
			__m128 box[2];
			box[0] = _mm_sub_ps(Center, HalfExtents);// Min
			box[1] = _mm_add_ps(Center, HalfExtents);// Max

			for (int i = 0; i < 6; i++)
			{
				__m128 a = Planes[i];
				__m128 b = _mm_setr_ps(box[Side[i][0]].m128_f32[0], box[Side[i][1]].m128_f32[1], box[Side[i][2]].m128_f32[2], 0.0f);

				// Compute dot and do a horizontal sum
				__m128 dp;
				dp = _mm_mul_ps(a, b);
				dp = _mm_hadd_ps(dp, dp);
				dp = _mm_hadd_ps(dp, dp);

				// if (dot prod < -plane.w)
				if (dp.m128_f32[0] < -a.m128_f32[3])
					return false;
			}

			return true;

			/*
			Still broken...sigh

			alignas(16) const static uint32_t signMask[4] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
			const __m128 xmm_signMask = _mm_load_ps((float *)&signMask);

			__m128 plane_x0 = _mm_setr_ps(Planes[0][0], Planes[1][0], Planes[2][0], Planes[3][0]);	// planes[0..3].x
			__m128 plane_x1 = _mm_setr_ps(Planes[4][0], Planes[5][0], 0.0f, 0.0f);					// planes[4..5].x

			__m128 plane_y0 = _mm_setr_ps(Planes[0][1], Planes[1][1], Planes[2][1], Planes[3][1]);	// planes[0..3].y
			__m128 plane_y1 = _mm_setr_ps(Planes[4][1], Planes[5][1], 0.0f, 0.0f);					// planes[4..5].y

			__m128 plane_z0 = _mm_setr_ps(Planes[0][2], Planes[1][2], Planes[2][2], Planes[3][2]);	// planes[0..3].z
			__m128 plane_z1 = _mm_setr_ps(Planes[4][2], Planes[5][2], 0.0f, 0.0f);					// planes[4..5].z

			__m128 plane_w0 = _mm_setr_ps(Planes[0][3], Planes[1][3], Planes[2][3], Planes[3][3]);	// planes[0..3].w
			__m128 plane_w1 = _mm_setr_ps(Planes[4][3], Planes[5][3], 0.0f, 0.0f);					// planes[4..5].w

			//plane_w0 = _mm_add_ps(plane_w0, plane_w0);// Multiplied by 2
			//plane_w1 = _mm_add_ps(plane_w1, plane_w1);// Multiplied by 2

			__m128 t0 = _mm_add_ps(XMVectorSplatZ(AABBCenter), _mm_xor_ps(XMVectorSplatZ(AABBExtents), _mm_and_ps(plane_z0, xmm_signMask)));
			__m128 t1 = _mm_add_ps(XMVectorSplatZ(AABBCenter), _mm_xor_ps(XMVectorSplatZ(AABBExtents), _mm_and_ps(plane_z1, xmm_signMask)));

			__m128 dot0 = simd_madd(t0, plane_z0, plane_w0);
			__m128 dot1 = simd_madd(t1, plane_z1, plane_w1);

			t0 = _mm_add_ps(XMVectorSplatY(AABBCenter), _mm_xor_ps(XMVectorSplatY(AABBExtents), _mm_and_ps(plane_y0, xmm_signMask)));
			t1 = _mm_add_ps(XMVectorSplatY(AABBCenter), _mm_xor_ps(XMVectorSplatY(AABBExtents), _mm_and_ps(plane_y1, xmm_signMask)));

			dot0 = simd_madd(t0, plane_y0, dot0);
			dot1 = simd_madd(t1, plane_y1, dot1);

			t0 = _mm_add_ps(XMVectorSplatX(AABBCenter), _mm_xor_ps(XMVectorSplatX(AABBExtents), _mm_and_ps(plane_x0, xmm_signMask)));
			t1 = _mm_add_ps(XMVectorSplatX(AABBCenter), _mm_xor_ps(XMVectorSplatX(AABBExtents), _mm_and_ps(plane_x1, xmm_signMask)));

			dot0 = simd_madd(t0, plane_x0, dot0);
			dot1 = simd_madd(t1, plane_x1, dot1);

			// si_nand(dot0, dot1)
			//__m128i nand;
			//nand = _mm_and_si128(*reinterpret_cast<__m128i *>(&dot0), *reinterpret_cast<__m128i *>(&dot1));
			//nand = _mm_cmpeq_epi32(nand, nand);
			//nand = _mm_xor_si128(nand, nand);

			__m128i ret;
			ret.m128i_u32[0] = ~(dot0.m128_u32[0] & dot1.m128_u32[0]);
			ret.m128i_u32[1] = ~(dot0.m128_u32[1] & dot1.m128_u32[1]);
			ret.m128i_u32[2] = ~(dot0.m128_u32[2] & dot1.m128_u32[2]);
			ret.m128i_u32[3] = ~(dot0.m128_u32[3] & dot1.m128_u32[3]);

			//__m128 test = si_orx(si_nand(dot0, dot1)); // all dots >= 0
			uint32_t test = ret.m128i_u32[0] | ret.m128i_u32[1] | ret.m128i_u32[2] | ret.m128i_u32[3];
			//return si_to_int(test) >> 31;
			return test >> 31;
			*/
		}

		__forceinline __m128 simd_madd(__m128 a, __m128 b, __m128 c)
		{
			return _mm_fmadd_ps(a, b, c);
			//return _mm_add_ps(_mm_mul_ps(a, b), c);
		}
	};
}