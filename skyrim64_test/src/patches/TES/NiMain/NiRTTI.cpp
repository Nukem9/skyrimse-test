#include "../../../common.h"
#include "NiRTTI.h"

std::map<std::string, const NiRTTI *> NiRTTIMap;

NiRTTI *NiRTTI::__ctor__(void *Instance, const char *Name, const NiRTTI *BaseType)
{
	return new (Instance) NiRTTI(Name, BaseType);
}

NiRTTI::NiRTTI(const char *Name, const NiRTTI *BaseType) : m_pcName(Name), m_pkBaseRTTI(BaseType)
{
	NiRTTIMap[Name] = this;

	switch (Profiler::Internal::CRC32(Name))
	{
#define DefineNiRTTI(x) case Profiler::Internal::CRC32(#x): ms_##x = this; break;
#include "NiRTTI.inl"
#undef DefineNiRTTI
	}
}

bool NiRTTI::CopyName(char* NameBuffer, uint32_t MaxSize) const
{
	const char* pcName = GetName();

	if (!pcName || !NameBuffer)
	{
		strcpy_s(NameBuffer, MaxSize, "\0");
		return false;
	}

	strcpy_s(NameBuffer, MaxSize, pcName);
	return true;
}

bool NiRTTI::Inherits(const NiRTTI *Other) const
{
	const NiRTTI *rtti = this;

	while (rtti != Other && rtti)
		rtti = rtti->GetBaseRTTI();

	return (rtti == Other);
}

const char *NiRTTI::GetName() const
{
	return m_pcName;
}

const NiRTTI *NiRTTI::GetBaseRTTI() const
{
	return m_pkBaseRTTI;
}

const std::map<std::string, const NiRTTI *>& NiRTTI::GetAllTypes()
{
	return NiRTTIMap;
}

void NiRTTI::DumpRTTIListing(FILE *File, bool IDAScript)
{
	if (IDAScript)
	{
		fprintf(File, "#include <idc.idc>\n");
		fprintf(File, "static main()\n");
		fprintf(File, "{\n");

		for (const auto& kv : NiRTTIMap)
		{
			void *addr = (void *)((uintptr_t)kv.second - g_ModuleBase + 0x140000000);
			fprintf(File, "\tcreate_qword(0x%p); set_name(0x%p, \"%s::ms_RTTI\");\n", addr, addr, kv.first.c_str());
		}

		fprintf(File, "}\n");
	}
	else
	{
		for (const auto& kv : NiRTTIMap)
			fprintf(File, "DefineNiRTTI(%s)\n", kv.first.c_str());
	}
}

#define DefineNiRTTI(x) NiRTTI *NiRTTI::ms_##x;
#include "NiRTTI.inl"
#undef DefineNiRTTI