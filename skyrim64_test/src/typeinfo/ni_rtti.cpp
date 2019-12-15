#include "../common.h"
#include <regex>
#include "../patches/TES/NiMain/NiRTTI.h"
#include "ms_rtti.h"

#define NI_START_LIST(x) const char *x##Functions[] = {
#define NI_LIST_ENTRY(x, ...) #x #__VA_ARGS__,
#define NI_END_LIST() nullptr };

#define NI_START_LIST_CK(x) const int x##CK[] = {
#define NI_LIST_ENTRY_CK(x) x,
#define NI_END_LIST_CK() -1 };

#include "ni_rtti.inl"

void AddTypes(std::vector<const char *>& Vector, const char **Functions, const int *CKFunctions)
{
	for (int i = 0; Functions[i]; i++)
	{
		bool allowInclude = true;

		// Skip any function indexes marked as CreationKit-only
		if (g_LoadType != GAME_EXECUTABLE_TYPE::CREATIONKIT_SKYRIM && g_LoadType != GAME_EXECUTABLE_TYPE::CREATIONKIT_FALLOUT4)
		{
			for (int j = 0; CKFunctions[j] != -1; j++)
			{
				if (CKFunctions[j] == i)
				{
					allowInclude = false;
					break;
				}
			}
		}

		if (allowInclude)
			Vector.push_back(Functions[i]);
	}
}

void ExportTest(FILE *File)
{
	auto inheritanceLevel = [](const NiRTTI *RTTI)
	{
		int level = 1;

		for (; RTTI; level++)
			RTTI = RTTI->GetBaseRTTI();

		return level;
	};

	// Convert the hash map to a vector & sort based on # of inheritance levels
	std::vector<const NiRTTI *> rttiVector;

	for (const auto& kv : NiRTTI::GetAllTypes())
		rttiVector.push_back(kv.second);

	std::sort(rttiVector.begin(), rttiVector.end(),
		[inheritanceLevel](const NiRTTI *& a, const NiRTTI *& b) -> bool
	{
		return inheritanceLevel(a) > inheritanceLevel(b);
	});

	// For each niRTTI entry
	for (const NiRTTI *niRTTI : rttiVector)
	{
		std::vector<const char *> vtableLayout;

		if (niRTTI->Inherits(NiRTTI::ms_NiObject))
		{
			AddTypes(vtableLayout, NiRefObjectFunctions, NiRefObjectCK);
			AddTypes(vtableLayout, NiObjectFunctions, NiObjectCK);

			if (niRTTI->Inherits(NiRTTI::ms_NiAVObject))
				AddTypes(vtableLayout, NiAVObjectFunctions, NiAVObjectCK);

			if (niRTTI->Inherits(NiRTTI::ms_bhkSerializable))
				AddTypes(vtableLayout, bhkSerializableFunctions, bhkSerializableCK);
		}
		else if (niRTTI->Inherits(NiRTTI::ms_NiCullingProcess))
		{
			AddTypes(vtableLayout, NiCullingProcessFunctions, NiCullingProcessCK);

			if (niRTTI->Inherits(NiRTTI::ms_BSCullingProcess))
				AddTypes(vtableLayout, BSCullingProcessFunctions, BSCullingProcessCK);
		}
		else
		{
			continue;
		}

		// Find the real RTTI information (for now, always select the vtable at offset 0)
		char buffer[256];
		sprintf_s(buffer, "class %s", niRTTI->GetName());

		const MSRTTI::Info *msRTTI = nullptr;

		for (auto& entry : MSRTTI::FindAll(buffer))
		{
			if (entry->VTableOffset == 0)
			{
				msRTTI = entry;
				break;
			}
		}

		if (!msRTTI)
			continue;

		// Reformat the strings with the specific types before printing
		for (size_t i = 0; i < vtableLayout.size(); i++)
		{
			std::string functionDecl = std::regex_replace(vtableLayout[i], std::regex("\\{\\{TYPE\\}\\}"), niRTTI->GetName());

			std::smatch sm;
			std::regex_search(functionDecl, sm, std::regex("\\{\\{(.*?)\\}\\}"));
			std::string functionName = std::string(niRTTI->GetName()) + std::string("::") + std::string(sm[1]);

			uintptr_t vfuncPointer = msRTTI->VTableAddress + (i * sizeof(uintptr_t));
			uintptr_t vfunc = *(uintptr_t *)vfuncPointer;

			// Make sure it's not a pure virtual function first
			if (*(uint8_t *)vfunc == 0xFF && *(uint8_t *)(vfunc + 1) == 0x25)
				functionName = "_purecall";

			fprintf(File, "set_name(0x%p, \"%s\");// %s\n", vfunc - g_ModuleBase + 0x140000000, functionName.c_str(), functionDecl.c_str());
		}

		fprintf(File, "\n\n");
	}
}