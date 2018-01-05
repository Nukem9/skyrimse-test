#pragma once

class NiMemObject
{
};

class NiPoint3
{
public:
	float x;
	float y;
	float z;

	inline NiPoint3()
	{
	}

	inline NiPoint3(float fX, float fY, float fZ) : x(fX), y(fY), z(fZ)
	{
	}

	inline NiPoint3(const NiPoint3& Src) : x(Src.x), y(Src.y), z(Src.z)
	{
	}

	inline void Unitize()
	{
		// AKA vector normalization
		float length = sqrt((x * x) + (y * y) + (z * z));

		if (length <= 0.000001f)
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}
		else
		{
			const float invMag = 1.0f / length;

			x *= invMag;
			y *= invMag;
			z *= invMag;
		}
	}

	inline NiPoint3 operator- () const
	{
		return NiPoint3(-x, -y, -z);
	}

	inline NiPoint3 operator* (float Scale) const
	{
		return NiPoint3(x * Scale, y * Scale, z * Scale);
	}
};
static_assert(sizeof(NiPoint3) == 0xC, "");

class NiMatrix3
{
public:
	float m_pEntry[3][3];

	inline NiMatrix3()
	{
	}

	inline NiMatrix3(const NiMatrix3& Src)
	{
		_mm_storeu_ps(&m_pEntry[0][0], _mm_loadu_ps(&Src.m_pEntry[0][0]));// 0 - 3
		_mm_storeu_ps(&m_pEntry[1][1], _mm_loadu_ps(&Src.m_pEntry[1][1]));// 4 - 7
		*(uint32_t *)&m_pEntry[2][2] = *(uint32_t *)&Src.m_pEntry[2][2];// 8 - 9
	}

	NiMatrix3 Transpose() const
	{
		// Swap [rows, cols] with [cols, rows]. Can only be optimized with AVX.
		NiMatrix3 m;

		m.m_pEntry[0][0] = m_pEntry[0][0];
		m.m_pEntry[0][1] = m_pEntry[1][0];
		m.m_pEntry[0][2] = m_pEntry[2][0];

		m.m_pEntry[1][0] = m_pEntry[0][1];
		m.m_pEntry[1][1] = m_pEntry[1][1];
		m.m_pEntry[1][2] = m_pEntry[2][1];
		
		m.m_pEntry[2][0] = m_pEntry[0][2];
		m.m_pEntry[2][1] = m_pEntry[1][2];
		m.m_pEntry[2][2] = m_pEntry[2][2];
		return m;
	}

	NiPoint3 operator* (const NiPoint3& Point) const
	{
		NiPoint3 p;

		p.x = m_pEntry[0][0] * Point.x + m_pEntry[0][1] * Point.y + m_pEntry[0][2] * Point.z;
		p.y = m_pEntry[1][0] * Point.x + m_pEntry[1][1] * Point.y + m_pEntry[1][2] * Point.z;
		p.z = m_pEntry[2][0] * Point.x + m_pEntry[2][1] * Point.y + m_pEntry[2][2] * Point.z;
		return p;
	}
};
static_assert(sizeof(NiMatrix3) == 0x24, "");

class NiTransform : public NiMemObject
{
public:
	NiMatrix3 m_Rotate;
	NiPoint3 m_Translate;
	float m_fScale;

	inline NiTransform()
	{
		m_fScale = 1.0f;
	}

	inline NiTransform(const NiTransform& Src) : m_Rotate(Src.m_Rotate), m_Translate(Src.m_Translate), m_fScale(Src.m_fScale)
	{
	}

	void Invert(NiTransform& Dest) const
	{
		Dest.m_Rotate = m_Rotate.Transpose();
		Dest.m_fScale = 1.0f / m_fScale;
		Dest.m_Translate = (Dest.m_Rotate * -m_Translate) * Dest.m_fScale;
	}
};
static_assert(sizeof(NiTransform) == 0x34, "");
static_assert(offsetof(NiTransform, m_Rotate) == 0x0, "");
static_assert(offsetof(NiTransform, m_Translate) == 0x24, "");
static_assert(offsetof(NiTransform, m_fScale) == 0x30, "");