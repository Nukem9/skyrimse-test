#pragma once

class NiRefObject
{
private:
	inline AutoPtr(int, ms_uiObjects, 0x3038520);
	unsigned int m_RefCount;

public:
	NiRefObject() : m_RefCount(0)
	{
		InterlockedIncrement((volatile long *)&ms_uiObjects);
	}

	virtual ~NiRefObject()
	{
		InterlockedDecrement((volatile long *)&ms_uiObjects);
	}

	virtual void DeleteThis()
	{
		__debugbreak();

		if (this)
			this->~NiRefObject();
	}
};
//static_assert(offsetof(NiRefObject, m_RefCount) == 0x8, "");