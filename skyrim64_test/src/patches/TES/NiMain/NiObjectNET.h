#pragma once

#include "NiObject.h"

struct BSFixedString
{
	void *ptr;

	BSFixedString(const char *String)
	{
		AutoFunc(void(*)(BSFixedString&, const char *), sub_140C28280, 0xC28280);
		sub_140C28280(*this, String);
	}

	~BSFixedString()
	{
		AutoFunc(void(*)(BSFixedString&), sub_140C283D0, 0xC283D0);
		sub_140C283D0(*this);
	}

	BSFixedString& operator= (BSFixedString &Other)
	{
		AutoFunc(void(*)(BSFixedString&, BSFixedString&), sub_140C284B0, 0xC284B0);
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

	void GetViewerStrings(void(*Callback)(const char *, ...), bool Recursive) const
	{
		if (Recursive)
			__super::GetViewerStrings(Callback, Recursive);

		Callback("-- NiObjectNET --\n");
		Callback("Name = %s\n", GetName()->c_str());
	}
};
static_assert(sizeof(NiObjectNET) == 0x30);
static_assert_offset(NiObjectNET, m_kName, 0x10);