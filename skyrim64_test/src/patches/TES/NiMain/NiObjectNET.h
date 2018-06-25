#pragma once

#include "NiObject.h"

struct BSFixedString
{
	void *ptr;

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

	const BSFixedString *GetName()
	{
		return &m_kName;
	}
};
static_assert(sizeof(NiObjectNET) == 0x30);
static_assert(offsetof(NiObjectNET, m_kName) == 0x10);