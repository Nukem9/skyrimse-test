#pragma once

class NiMemObject
{
};

class NiMatrix3
{
public:
	float m_pEntry[3][3];
};

class NiPoint3
{
public:
	float x;
	float y;
	float z;

	inline NiPoint3(float fX, float fY, float fZ) : x(fX), y(fY), z(fZ)
	{
	}

	inline NiPoint3(const NiPoint3& Src) : x(Src.x), y(Src.y), z(Src.z)
	{
	}

	void Unitize()
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
			const float m = 1.0f / length;

			x *= m;
			y *= m;
			z *= m;
		}
	}
};
static_assert(sizeof(NiPoint3) == 0xC, "");

class NiTransform// : public NiMemObject
{
public:
	NiMatrix3 m_Rotate;
	NiPoint3 m_Translate;
	float m_fScale;
};
static_assert(sizeof(NiTransform) == 0x34, "");
//static_assert(offsetof(NiTransform, m_Rotate) == 0x0, "");
//static_assert(offsetof(NiTransform, m_Translate) == 0x24, "");
//static_assert(offsetof(NiTransform, m_fScale) == 0x30, "");