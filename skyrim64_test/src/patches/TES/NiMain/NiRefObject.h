#pragma once

class NiRefObject
{
private:
	inline AutoPtr(uint32_t, ms_uiObjects, 0x3038520);
	uint32_t m_uiRefCount;

public:
	NiRefObject() : m_uiRefCount(0)
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
		return InterlockedIncrement(&m_uiRefCount);
	}

	uint32_t DecRefCount()
	{
		uint32_t count = InterlockedDecrement(&m_uiRefCount);

		if (count <= 0)
			DeleteThis();

		return count;
	}

	void GetViewerStrings(void(*Callback)(const char *, ...), bool Recursive) const
	{
		Callback("-- NiRefObject --\n");
		Callback("This = 0x%p\n", this);
		Callback("Ref Count = %u\n", m_uiRefCount);
	}

	static uint32_t GetTotalObjectCount()
	{
		return ms_uiObjects;
	}
};
static_assert(sizeof(NiRefObject) == 0x10);
//static_assert_offset(NiRefObject, m_uiRefCount, 0x8);