#pragma once

#include <DirectXMath.h>
#include "NiAVObject.h"
#include "NiFrustum.h"

template<typename T>
struct NiRect
{
	T m_left;
	T m_right;
	T m_top;
	T m_bottom;
};

class NiCamera : public NiAVObject
{
public:
	float m_aafWorldToCam[4][4];
	NiFrustum m_kViewFrustum;
	float m_fMinNearPlaneDist;
	float m_fMaxFarNearRatio;
	NiRect<float> m_kPort;
	char _pad0[0x4];

	inline const NiPoint3& NiCamera::GetWorldLocation() const
	{
		return m_kWorld.m_Translate;
	}

	inline NiPoint3 NiCamera::GetWorldDirection() const
	{
		return m_kWorld.m_Rotate.GetCol<0>();
	}

	inline NiPoint3 NiCamera::GetWorldUpVector() const
	{
		return m_kWorld.m_Rotate.GetCol<1>();
	}

	inline NiPoint3 NiCamera::GetWorldRightVector() const
	{
		return m_kWorld.m_Rotate.GetCol<2>();
	}

	void CalculateViewProjection(DirectX::XMMATRIX& ViewProj) const
	{
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;

		CalculateViewProjection(view, projection, ViewProj);
	}

	void CalculateViewProjection(DirectX::XMMATRIX& View, DirectX::XMMATRIX& Proj, DirectX::XMMATRIX& ViewProj) const
	{
		// Ported directly from game code
		NiPoint3 dir = GetWorldDirection();
		NiPoint3 up = GetWorldUpVector();
		NiPoint3 right = GetWorldRightVector();

		View.r[0] = { right.x, up.x, dir.x, 0.0f };
		View.r[1] = { right.y, up.y, dir.y, 0.0f };
		View.r[2] = { right.z, up.z, dir.z, 0.0f };
		View.r[3] = { 0.0f, 0.0f, 0.0f, 1.0f };

		float rightLeftDiff = m_kViewFrustum.m_fRight - m_kViewFrustum.m_fLeft;
		float rightLeftSum = m_kViewFrustum.m_fRight + m_kViewFrustum.m_fLeft;
		float rightLeftRatio = -((1.0f / rightLeftDiff) * rightLeftSum);

		float topBottomDiff = m_kViewFrustum.m_fTop - m_kViewFrustum.m_fBottom;
		float topBottomSum = m_kViewFrustum.m_fTop + m_kViewFrustum.m_fBottom;
		float topBottomRatio = -((1.0f / topBottomDiff) * topBottomSum);

		float invNearFarDiff = 1.0f / (m_kViewFrustum.m_fFar - m_kViewFrustum.m_fNear);

		Proj.r[0] = _mm_setzero_ps();
		Proj.r[1] = _mm_setzero_ps();
		Proj.r[2] = _mm_setzero_ps();
		Proj.r[3] = _mm_setzero_ps();

		Proj.r[0].m128_f32[0] = (1.0f / rightLeftDiff) * 2.0f;
		Proj.r[1].m128_f32[1] = (1.0f / topBottomDiff) * 2.0f;

		if (!m_kViewFrustum.m_bOrtho)
		{
			Proj.r[2].m128_f32[0] = rightLeftRatio;
			Proj.r[2].m128_f32[1] = topBottomRatio;
			Proj.r[2].m128_f32[2] = invNearFarDiff * m_kViewFrustum.m_fFar;
			Proj.r[2].m128_f32[3] = 1.0f;

			float ratio = m_kViewFrustum.m_fNear * m_kViewFrustum.m_fFar;
			Proj.r[3].m128_f32[2] = -(ratio * invNearFarDiff);
		}
		else
		{
			Proj.r[2].m128_f32[2] = invNearFarDiff;

			Proj.r[3].m128_f32[0] = rightLeftRatio;
			Proj.r[3].m128_f32[1] = topBottomRatio;
			Proj.r[3].m128_f32[2] = -(invNearFarDiff * m_kViewFrustum.m_fNear);
			Proj.r[3].m128_f32[3] = 1.0f;
		}

		ViewProj = XMMatrixMultiply(View, Proj);
	}

	bool WorldToScreen(const NiPoint3& WorldPoint, float& X, float& Y, float fZeroTolerance = 1e-5) const
	{
		// Returns X and Y as a percentage of the viewport width and height
		fZeroTolerance = max(fZeroTolerance, 0.0f);

		// Project a world space point to screen space
		float fW =
			WorldPoint.x * m_aafWorldToCam[3][0] +
			WorldPoint.y * m_aafWorldToCam[3][1] +
			WorldPoint.z * m_aafWorldToCam[3][2] +
			m_aafWorldToCam[3][3];

		// Check to see if we're on the appropriate side of the camera.
		if (fW > fZeroTolerance)
		{
			float fInvW = 1.0f / fW;

			X = WorldPoint.x * m_aafWorldToCam[0][0] + WorldPoint.y * m_aafWorldToCam[0][1] +
				WorldPoint.z * m_aafWorldToCam[0][2] + m_aafWorldToCam[0][3];
			Y = WorldPoint.x * m_aafWorldToCam[1][0] + WorldPoint.y * m_aafWorldToCam[1][1] +
				WorldPoint.z * m_aafWorldToCam[1][2] + m_aafWorldToCam[1][3];

			X = X * fInvW;
			Y = Y * fInvW;

			X *= (m_kPort.m_right - m_kPort.m_left) * 0.5f;
			Y *= (m_kPort.m_top - m_kPort.m_bottom) * 0.5f;

			X += (m_kPort.m_right + m_kPort.m_left) * 0.5f;
			Y += (m_kPort.m_top + m_kPort.m_bottom) * 0.5f;

			// If on screen return true. Otherwise, we fall through to false.
			if (X >= m_kPort.m_left && X <= m_kPort.m_right &&
				Y >= m_kPort.m_bottom && Y <= m_kPort.m_top)
			{
				return true;
			}
		}

		return false;
	}

	void GetViewerStrings(void(*Callback)(const char *, ...), bool Recursive) const
	{
		if (Recursive)
			__super::GetViewerStrings(Callback, Recursive);

		Callback("-- NiCamera --\n");
		Callback("World Dir = (%g, %g, %g)\n",
			GetWorldDirection().x,
			GetWorldDirection().y,
			GetWorldDirection().z);
		Callback("World Up = (%g, %g, %g)\n",
			GetWorldUpVector().x,
			GetWorldUpVector().y,
			GetWorldUpVector().z);
		Callback("World Right = (%g, %g, %g)\n",
			GetWorldRightVector().x,
			GetWorldRightVector().y,
			GetWorldRightVector().z);
	}
};
static_assert(sizeof(NiCamera) == 0x188);
static_assert_offset(NiCamera, m_aafWorldToCam, 0x110);
static_assert_offset(NiCamera, m_kViewFrustum, 0x150);
static_assert_offset(NiCamera, m_kPort, 0x174);