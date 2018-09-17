#pragma once

#include "NiRefObject.h"

template<typename T>
class NiPointer
{
protected:
	T *m_pObject;

public:
	static_assert(std::is_base_of_v<NiRefObject, T>, "T must inherit NiRefObject");

	inline NiPointer(T *Object = (T *)nullptr)
	{
		m_pObject = Object;

		if (m_pObject)
			static_cast<NiRefObject *>(m_pObject)->IncRefCount();
	}

	inline NiPointer(const NiPointer<T>& Other)
	{
		m_pObject = Other.m_pObject;

		if (m_pObject)
			static_cast<NiRefObject *>(m_pObject)->IncRefCount();
	}

	inline ~NiPointer()
	{
		if (m_pObject)
			static_cast<NiRefObject *>(m_pObject)->DecRefCount();
	}

	inline operator T*() const
	{
		return m_pObject;
	}

	inline T& operator*() const
	{
		return m_pObject;
	}

	inline T* operator->() const
	{
		return m_pObject;
	}
};
static_assert(sizeof(NiPointer<NiRefObject>) == 0x8);