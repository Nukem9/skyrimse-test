#pragma once

class NiRTTI
{
private:
	const char *m_pcName;
	const NiRTTI *m_pkBaseRTTI;

public:
	static NiRTTI *__ctor__(void *Instance, const char *Name, const NiRTTI *BaseType);
	NiRTTI(const char *Name, const NiRTTI *BaseType);

	const char *GetName() const;
	const NiRTTI *GetBaseRTTI() const;

	static void DumpRTTIListing(FILE *File, bool IDAScript);

#define DefineNiRTTI(x) static NiRTTI *ms_##x;
#include "NiRTTI.inl"
#undef DefineNiRTTI
};
static_assert(sizeof(NiRTTI) == 0x10);
//static_assert(offsetof(NiRTTI, m_pcName) == 0x0);
//static_assert(offsetof(NiRTTI, m_pkBaseRTTI) == 0x10);