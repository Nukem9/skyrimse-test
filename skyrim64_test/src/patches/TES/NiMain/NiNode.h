#pragma once

#include "NiAVObject.h"

template<typename T>
class NiTArray
{
public:
	virtual ~NiTArray();

	T *m_pBase;				// Sparse array (nullptr for invalid)
	uint16_t m_usMaxSize;	// Number of elements allocated
	uint16_t m_usSize;		// Last index with a valid element
	uint16_t m_usESize;		// Number of elements in use
	uint16_t m_usGrowBy;

	uint16_t GetMaxSize() const
	{
		return m_usMaxSize;
	}

	uint16_t GetSize() const
	{
		return m_usSize;
	}

	uint16_t GetESize() const
	{
		return m_usESize;
	}

	T At(size_t Index) const
	{
		return m_pBase[Index];
	}
};

template<typename T>
class NiTObjectArray : public NiTArray<T>
{
public:
	virtual ~NiTObjectArray();
};

class NiNode : public NiAVObject
{
public:
	NiTObjectArray<NiAVObject *> m_kChildren;

	uint32_t GetMaxSize() const
	{
		return m_kChildren.GetMaxSize();
	}

	uint32_t GetArrayCount() const
	{
		return m_kChildren.GetSize();
	}

	uint32_t GetChildCount() const
	{
		return m_kChildren.GetESize();
	}

	NiAVObject *GetAt(size_t Index) const
	{
		return m_kChildren.At(Index);
	}
};
static_assert(sizeof(NiNode) == 0x128);
static_assert(offsetof(NiNode, m_kChildren) == 0x110);