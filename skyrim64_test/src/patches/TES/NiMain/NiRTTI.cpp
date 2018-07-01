#include <map>
#include "../../../common.h"
#include "NiRTTI.h"

std::map<std::string, const NiRTTI *> NiRTTIMap;

NiRTTI *NiRTTI::__ctor__(void *Instance, const char *Name, const NiRTTI *BaseType)
{
	return new (Instance) NiRTTI(Name, BaseType);
}

NiRTTI::NiRTTI(const char *Name, const NiRTTI *BaseType)
{
	m_pcName = Name;
	m_pkBaseRTTI = BaseType;

	NiRTTIMap[Name] = BaseType;

	switch (Profiler::Internal::CRC32(Name))
	{
#define DefineNiRTTI(x) case Profiler::Internal::CRC32(#x): ms_##x = this; break;
#include "NiRTTI.inl"
#undef DefineNiRTTI
	}
}

const char *NiRTTI::GetName() const
{
	return m_pcName;
}

const NiRTTI *NiRTTI::GetBaseRTTI() const
{
	return m_pkBaseRTTI;
}

void NiRTTI::DumpRTTIListing(FILE *File)
{
	for (const auto& kv : NiRTTIMap)
		fprintf(File, "DefineNiRTTI(%s)\n", kv.first.c_str());
}

#define DefineNiRTTI(x) NiRTTI *NiRTTI::ms_##x;
#include "NiRTTI.inl"
#undef DefineNiRTTI