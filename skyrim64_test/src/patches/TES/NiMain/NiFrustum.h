#pragma once

struct NiFrustum
{
	float m_fLeft;
	float m_fRight;
	float m_fTop;
	float m_fBottom;
	float m_fNear;
	float m_fFar;
	bool m_bOrtho;
};
static_assert_offset(NiFrustum, m_fLeft, 0x0);
static_assert_offset(NiFrustum, m_fRight, 0x4);
static_assert_offset(NiFrustum, m_fTop, 0x8);
static_assert_offset(NiFrustum, m_fBottom, 0xC);
static_assert_offset(NiFrustum, m_fNear, 0x10);
static_assert_offset(NiFrustum, m_fFar, 0x14);
static_assert_offset(NiFrustum, m_bOrtho, 0x18);