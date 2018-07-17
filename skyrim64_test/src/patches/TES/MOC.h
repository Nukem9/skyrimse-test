#pragma once

#include <DirectXMath.h>

namespace MOC
{
	void Init();
	bool RegisterGeo(BSGeometry *Geometry, bool Test, bool Draw);
	void SendTraverseCommand(NiCamera *Camera);
	void TraverseSceneGraph(NiCamera *Camera);
	void RemoveCachedVerticesAndIndices(void *RendererData);
	void UpdateDepthViewTexture();
	void ForceFlush();

	bool TestObject(NiAVObject *Object);

	using namespace DirectX;

	struct alignas(16) fplanes
	{
		union
		{
			float Planes[6][4];
			XMVECTOR XMPlanes[6];
		};

		__m128 PlaneComponents[8];

		void CreateFromViewProjMatrix(XMMATRIX M)
		{
			XMVECTOR col1 = XMVectorSet(M.r[0].m128_f32[0], M.r[1].m128_f32[0], M.r[2].m128_f32[0], M.r[3].m128_f32[0]);// { _11, _21, _31, _41 }
			XMVECTOR col2 = XMVectorSet(M.r[0].m128_f32[1], M.r[1].m128_f32[1], M.r[2].m128_f32[1], M.r[3].m128_f32[1]);// { _12, _22, _32, _42 }
			XMVECTOR col3 = XMVectorSet(M.r[0].m128_f32[2], M.r[1].m128_f32[2], M.r[2].m128_f32[2], M.r[3].m128_f32[2]);// { _13, _23, _33, _43 }
			XMVECTOR col4 = XMVectorSet(M.r[0].m128_f32[3], M.r[1].m128_f32[3], M.r[2].m128_f32[3], M.r[3].m128_f32[3]);// { _14, _24, _34, _44 }

			XMPlanes[0] = XMPlaneNormalize(XMVectorAdd(col4, col1));		// [c4 + c1] Left
			XMPlanes[1] = XMPlaneNormalize(XMVectorSubtract(col4, col1));	// [c4 - c1] Right
			XMPlanes[2] = XMPlaneNormalize(XMVectorSubtract(col4, col2));	// [c4 - c2] Top
			XMPlanes[3] = XMPlaneNormalize(XMVectorAdd(col4, col2));		// [c4 + c2] Bottom
			XMPlanes[4] = XMPlaneNormalize(XMVectorAdd(col4, col3));		// [c4 + c3] Near
			XMPlanes[5] = XMPlaneNormalize(XMVectorSubtract(col4, col3));	// [c4 - c3] Far

			UpdateSphereOptimization();
		}

		void UpdateSphereOptimization()
		{
			PlaneComponents[0] = _mm_setr_ps(-Planes[0][0], -Planes[1][0], -Planes[2][0], -Planes[3][0]);
			PlaneComponents[1] = _mm_setr_ps(-Planes[0][1], -Planes[1][1], -Planes[2][1], -Planes[3][1]);
			PlaneComponents[2] = _mm_setr_ps(-Planes[0][2], -Planes[1][2], -Planes[2][2], -Planes[3][2]);
			PlaneComponents[3] = _mm_setr_ps(-Planes[0][3], -Planes[1][3], -Planes[2][3], -Planes[3][3]);
			PlaneComponents[4] = _mm_setr_ps(-Planes[4][0], -Planes[5][0], -Planes[4][0], -Planes[5][0]);
			PlaneComponents[5] = _mm_setr_ps(-Planes[4][1], -Planes[5][1], -Planes[4][1], -Planes[5][1]);
			PlaneComponents[6] = _mm_setr_ps(-Planes[4][2], -Planes[5][2], -Planes[4][2], -Planes[5][2]);
			PlaneComponents[7] = _mm_setr_ps(-Planes[4][3], -Planes[5][3], -Planes[4][3], -Planes[5][3]);
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

		bool TestAABB(XMVECTOR AABBCenter, XMVECTOR AABBExtents)
		{
			__declspec(align(16)) const static uint32_t absPlaneMask[4] = { 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF };

			const __m128 xmm_absPlaneMask = _mm_load_ps((float *)&absPlaneMask);
			const __m128 aabbCenter = AABBCenter;
			const __m128 aabbExtent = AABBExtents;

			// Assume that the aabb will be inside the frustum
			bool visible = true;

			for (int i = 0; i < 6; i++)
			{
				//
				// p_ = plane
				// a_ = abs(plane)
				// c_ = AABB center
				// e_ = AABB extent
				//
				const __m128 plane = XMPlanes[i];
				__m128 d;
				__m128 r;

				// d = { p_x, p_y, p_z, p_d } * { c_x, c_y, c_z, 0 }
				// d = sum(d);
				d = _mm_mul_ps(plane, aabbCenter);
				d = _mm_hadd_ps(d, d);
				d = _mm_hadd_ps(d, d);

				// r = { a_x, a_y, a_z, a_d } * { e_x, e_y, e_z, 0 }
				// r = sum(r);
				r = _mm_mul_ps(_mm_and_ps(plane, xmm_absPlaneMask), aabbExtent);
				r = _mm_hadd_ps(r, r);
				r = _mm_hadd_ps(r, r);

				__m128 plane_d = XMVectorSplatW(plane);
				__m128 xmm_d_p_r = _mm_add_ss(_mm_add_ss(d, r), plane_d);
				__m128 xmm_d_m_r = _mm_add_ss(_mm_sub_ss(d, r), plane_d);

				// Shuffle d_p_r and d_m_r in order to perform only one _mm_movmask_ps
				__m128 xmm_d_p_r__d_m_r = _mm_shuffle_ps(xmm_d_p_r, xmm_d_m_r, _MM_SHUFFLE(0, 0, 0, 0));
				int negativeMask = _mm_movemask_ps(xmm_d_p_r__d_m_r);

				// Bit 0 holds the sign of d + r and bit 2 holds the sign of d - r
				if (negativeMask & 0x1)
				{
					// Completely out of frustum
					visible = false;
					break;
				}
				else if (negativeMask & 0x4)
				{
					// Intersection
					visible = true;
				}
			}

			return !visible;
		}

		__forceinline __m128 simd_madd(__m128 a, __m128 b, __m128 c)
		{
			return _mm_fmadd_ps(a, b, c);
			//return _mm_add_ps(_mm_mul_ps(a, b), c);
		}
	};
}