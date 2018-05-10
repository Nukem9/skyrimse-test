#pragma once

class NiRefObject
{
private:
	inline AutoPtr(uint32_t, ms_uiObjects, 0x3038520);
	uint32_t m_RefCount;

public:
	NiRefObject() : m_RefCount(0)
	{
		InterlockedIncrement(&ms_uiObjects);
	}

	virtual ~NiRefObject()
	{
		InterlockedDecrement(&ms_uiObjects);
	}

	virtual void DeleteThis()
	{
		if (this)
			this->~NiRefObject();
	}

	uint32_t IncRefCount()
	{
		return InterlockedIncrement(&m_RefCount);
	}

	uint32_t DecRefCount()
	{
		uint32_t count = InterlockedDecrement(&m_RefCount);

		if (count <= 0)
			DeleteThis();

		return count;
	}

	static uint32_t GetTotalObjectCount()
	{
		return ms_uiObjects;
	}
};
static_assert(sizeof(NiRefObject) == 0x10);
//static_assert(offsetof(NiRefObject, m_RefCount) == 0x8, "");