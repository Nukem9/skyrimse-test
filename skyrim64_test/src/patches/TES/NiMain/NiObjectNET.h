#pragma once

#include "NiObject.h"

struct BSFixedString
{
	void *ptr;

	BSFixedString(const char *String)
	{
		((void(*)(BSFixedString&, const char *))(g_ModuleBase + 0xC28280))(*this, String);
	}

	~BSFixedString()
	{
		((void(*)(BSFixedString&))(g_ModuleBase + 0xC283D0))(*this);
	}

	BSFixedString& operator= (BSFixedString &Other)
	{
		((void(*)(BSFixedString&, BSFixedString&))(g_ModuleBase + 0xC284B0))(*this, Other);
		return *this;
	}

	const char *c_str() const
	{
		return (const char *)ptr;
	}
};

class NiObjectNET : public NiObject
{
public:
	virtual ~NiObjectNET();

	BSFixedString m_kName;
	char _pad[0x18];

	const BSFixedString *GetName() const
	{
		return &m_kName;
	}

	void SetName(BSFixedString& Name)
	{
		m_kName = Name;
	}
};
static_assert(sizeof(NiObjectNET) == 0x30);
static_assert(offsetof(NiObjectNET, m_kName) == 0x10);