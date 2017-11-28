#pragma once

// Verified
class NiRefObject
{
private:
	//static int ms_uiObjects;
	unsigned int m_RefCount;

public:
	NiRefObject() : m_RefCount(0)
	{
		__debugbreak();
		// InterlockedIncrement(&NiRefObject::ms_uiObjects);
	}

	virtual ~NiRefObject()
	{
		__debugbreak();
		// InterlockedDecrement(&NiRefObject::ms_uiObjects);
	}

	virtual void DeleteThis()
	{
		__debugbreak();

		if (this)
			this->~NiRefObject();
	}
};

//static_assert(offsetof(NiRefObject, m_RefCount) == 0x8, "");