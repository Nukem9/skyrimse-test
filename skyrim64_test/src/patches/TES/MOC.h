#pragma once

#include <immintrin.h>
#include <smmintrin.h>
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
		enum Plane
		{
			Near,
			Left,
			Top,
			Bottom,
			Right,
			Far,
		};

		alignas(64) __m128 PlaneComponents[8];
		alignas(64) float mPlanes[32];

		void InitializeFrustumAABB(float nearClipDistance, float farClipDistance, float aspectRatio, float fov, XMVECTOR position, XMVECTOR look, XMVECTOR up)
		{
			position = _mm_setzero_ps();

			// We have the camera's up and look, but we also need right.
			XMVECTOR right = XMVector3Cross(up, look);

			// Compute the position of the center of the near and far clip planes.
			XMVECTOR nearCenter = position + look * nearClipDistance;
			XMVECTOR farCenter = position + look * farClipDistance;

			// Compute the width and height of the near and far clip planes
			float tanHalfFov = tanf(0.5f * fov);
			float halfNearWidth = nearClipDistance * tanHalfFov;
			float halfNearHeight = halfNearWidth / aspectRatio;

			float halfFarWidth = farClipDistance * tanHalfFov;
			float halfFarHeight = halfFarWidth / aspectRatio;

			// Create two vectors each for the near and far clip planes.
			// These are the scaled up and right vectors.
			XMVECTOR upNear = up * halfNearHeight;
			XMVECTOR rightNear = right * halfNearWidth;
			XMVECTOR upFar = up * halfFarHeight;
			XMVECTOR rightFar = right * halfFarWidth;

			// Use the center positions and the up and right vectors
			// to compute the positions for the near and far clip plane vertices (four each)
			XMVECTOR mpPosition[8];
			mpPosition[0] = nearCenter + upNear - rightNear;// near top left
			mpPosition[1] = nearCenter + upNear + rightNear;// near top right
			mpPosition[2] = nearCenter - upNear + rightNear;// near bottom right
			mpPosition[3] = nearCenter - upNear - rightNear;// near bottom left
			mpPosition[4] = farCenter + upFar - rightFar;	// far top left
			mpPosition[5] = farCenter + upFar + rightFar;	// far top right
			mpPosition[6] = farCenter - upFar + rightFar;	// far bottom right
			mpPosition[7] = farCenter - upFar - rightFar;	// far bottom left

			// Compute some of the frustum's edge vectors.  We will cross these
			// to get the normals for each of the six planes.
			XMVECTOR nearTop = mpPosition[1] - mpPosition[0];
			XMVECTOR nearLeft = mpPosition[3] - mpPosition[0];
			XMVECTOR topLeft = mpPosition[4] - mpPosition[0];
			XMVECTOR bottomRight = mpPosition[2] - mpPosition[6];
			XMVECTOR farRight = mpPosition[5] - mpPosition[6];
			XMVECTOR farBottom = mpPosition[7] - mpPosition[6];

			// Clip plane normals
			XMVECTOR mpNormal[6];
			mpNormal[Plane::Near] = XMVector3Normalize(XMVector3Cross(nearTop, nearLeft));			// Near
			mpNormal[Plane::Left] = XMVector3Normalize(XMVector3Cross(nearLeft, topLeft));			// Left
			mpNormal[Plane::Top] = XMVector3Normalize(XMVector3Cross(topLeft, nearTop));			// Top
			mpNormal[Plane::Bottom] = XMVector3Normalize(XMVector3Cross(farBottom, bottomRight));	// Bottom
			mpNormal[Plane::Right] = XMVector3Normalize(XMVector3Cross(bottomRight, farRight));		// Right
			mpNormal[Plane::Far] = XMVector3Normalize(XMVector3Cross(farRight, farBottom));			// Far

			// For each (frustum vertex)
			for (int i = 0; i < 8; i++)
			{
				if (i < 6)
				{
					mPlanes[0 * 8 + i] = mpNormal[i].m128_f32[0];
					mPlanes[1 * 8 + i] = mpNormal[i].m128_f32[1];
					mPlanes[2 * 8 + i] = mpNormal[i].m128_f32[2];
					mPlanes[3 * 8 + i] = -XMVector3Dot(mpNormal[i], mpPosition[(i < 3) ? 0 : 6]).m128_f32[0];
				}
				else
				{
					mPlanes[0 * 8 + i] = 0;
					mPlanes[1 * 8 + i] = 0;
					mPlanes[2 * 8 + i] = 0;
					mPlanes[3 * 8 + i] = -1.0f;
				}
			}
		}

		void InitializeFrustumSphere()
		{
		}

		void CreateFromViewProjMatrix(XMMATRIX M)
		{
			M = XMMatrixTranspose(M);
			XMVECTOR col1 = M.r[0];// { _11, _21, _31, _41 }
			XMVECTOR col2 = M.r[1];// { _12, _22, _32, _42 }
			XMVECTOR col3 = M.r[2];// { _13, _23, _33, _43 }
			XMVECTOR col4 = M.r[3];// { _14, _24, _34, _44 }

			XMVECTOR Planes[6];
			Planes[0] = XMPlaneNormalize(XMVectorAdd(col4, col1));		// [c4 + c1] Left
			Planes[1] = XMPlaneNormalize(XMVectorSubtract(col4, col1));	// [c4 - c1] Right
			Planes[2] = XMPlaneNormalize(XMVectorSubtract(col4, col2));	// [c4 - c2] Top
			Planes[3] = XMPlaneNormalize(XMVectorAdd(col4, col2));		// [c4 + c2] Bottom
			Planes[4] = XMPlaneNormalize(XMVectorAdd(col4, col3));		// [c4 + c3] Near
			Planes[5] = XMPlaneNormalize(XMVectorSubtract(col4, col3));	// [c4 - c3] Far

			// Sphere test optimization
			const auto planes = (float(*)[4])&Planes;

			PlaneComponents[0] = _mm_setr_ps(-planes[0][0], -planes[1][0], -planes[2][0], -planes[3][0]);
			PlaneComponents[1] = _mm_setr_ps(-planes[0][1], -planes[1][1], -planes[2][1], -planes[3][1]);
			PlaneComponents[2] = _mm_setr_ps(-planes[0][2], -planes[1][2], -planes[2][2], -planes[3][2]);
			PlaneComponents[3] = _mm_setr_ps(-planes[0][3], -planes[1][3], -planes[2][3], -planes[3][3]);
			PlaneComponents[4] = _mm_setr_ps(-planes[4][0], -planes[5][0], -planes[4][0], -planes[5][0]);
			PlaneComponents[5] = _mm_setr_ps(-planes[4][1], -planes[5][1], -planes[4][1], -planes[5][1]);
			PlaneComponents[6] = _mm_setr_ps(-planes[4][2], -planes[5][2], -planes[4][2], -planes[5][2]);
			PlaneComponents[7] = _mm_setr_ps(-planes[4][3], -planes[5][3], -planes[4][3], -planes[5][3]);
		}

		bool SphereInFrustum(float Center[3], float Radius)
		{
			return SphereInFrustum(_mm_setr_ps(Center[0], Center[1], Center[2], Radius));
		}

		bool SphereInFrustum(XMVECTOR Center_X_Y_Z_Radius)
		{
			//
			// https://github.com/nsf/sseculling/blob/master/Common.cpp#L71 by "nsf"
			//
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
			return _mm_test_all_ones(*reinterpret_cast<__m128i *>(&r)) == 0;
		}

		bool AABBInFrustum(XMVECTOR Center, XMVECTOR HalfExtents)
		{
			const __m128 centerX = XMVectorSplatX(Center);
			const __m128 centerY = XMVectorSplatY(Center);
			const __m128 centerZ = XMVectorSplatZ(Center);

			const __m128 extents = _mm_add_ps(HalfExtents, HalfExtents);
			const __m128 halfX = XMVectorSplatX(extents);
			const __m128 halfY = XMVectorSplatY(extents);
			const __m128 halfZ = XMVectorSplatZ(extents);

			const __m128 signMask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));

			// Test the bounding box against 4 planes at a time
			for (int i = 0; i < 2; i++)
			{
				__m128 planesX = _mm_load_ps(&mPlanes[0 * 8 + i * 4]);
				__m128 planesY = _mm_load_ps(&mPlanes[1 * 8 + i * 4]);
				__m128 planesZ = _mm_load_ps(&mPlanes[2 * 8 + i * 4]);
				__m128 planesW = _mm_load_ps(&mPlanes[3 * 8 + i * 4]);

				// Sign for half[XYZ] so that dot product with plane normal would be maximal
				__m128 halfXSgn = _mm_xor_ps(halfX, _mm_and_ps(planesX, signMask));
				__m128 halfYSgn = _mm_xor_ps(halfY, _mm_and_ps(planesY, signMask));
				__m128 halfZSgn = _mm_xor_ps(halfZ, _mm_and_ps(planesZ, signMask));

				// Bounding box corner to test (min corner)
				__m128 cornerX = _mm_sub_ps(centerX, halfXSgn);
				__m128 cornerY = _mm_sub_ps(centerY, halfYSgn);
				__m128 cornerZ = _mm_sub_ps(centerZ, halfZSgn);

				// Dot product
				__m128 dot;
				dot = simd_madd(cornerX, planesX, planesW);
				dot = simd_madd(cornerY, planesY, dot);
				dot = simd_madd(cornerZ, planesZ, dot);

				// If not all negative, at least one plane rejected the box completely
				if (!_mm_testc_si128(_mm_castps_si128(dot), _mm_castps_si128(signMask)))
					return false;
			}

			return true;
		}

		__forceinline __m128 simd_madd(__m128 a, __m128 b, __m128 c)
		{
			return _mm_fmadd_ps(a, b, c);
			//return _mm_add_ps(_mm_mul_ps(a, b), c);
		}
	};
}