#include <regex>
#include "../common.h"
#include "../patches/TES/NiMain/NiRTTI.h"
#include "ms_rtti.h"

// NiRefObject
const char *NiRefObjectFunctions[] =
{
	"void *{{__vecDelDtor}}({{TYPE}} *this, unsigned int)",
	"void {{DeleteThis}}({{TYPE}} *this)",
	nullptr,
};

const int NiRefObjectCK[] =
{
	-1,
};

// NiObject
const char *NiObjectFunctions[] =
{
	"NiRTTI *{{GetRTTI}}({{TYPE}} *this)",
	"NiNode *{{IsNode}}({{TYPE}} *this)",
	"NiSwitchNode *{{IsSwitchNode}}({{TYPE}} *this)",
	"BSFadeNode *{{IsFadeNode}}({{TYPE}} *this)",
	"BSMultiBoundNode *{{IsMultiBoundNode}}({{TYPE}} *this)",
	"BSGeometry *{{IsGeometry}}({{TYPE}} *this)",
	"NiTriStrips *{{IsTriStrips}}({{TYPE}} *this)",
	"BSTriShape *{{IsTriShape}}({{TYPE}} *this)",
	"BSSegmentedTriShape *{{IsSegmentedTriShape}}({{TYPE}} *this)",
	"BSSubIndexTriShape *{{IsSubIndexTriShape}}({{TYPE}} *this)",
	"BSDynamicTriShape *{{IsDynamicTriShape}}({{TYPE}} *this)",
	"NiGeometry *{{IsNiGeometry}}({{TYPE}} *this)",
	"NiTriBasedGeom *{{IsNiTriBasedGeom}}({{TYPE}} *this)",
	"NiTriShape *{{IsNiTriShape}}({{TYPE}} *this)",
	"NiParticles *{{IsParticlesGeom}}({{TYPE}} *this)",
	"BSLines *{{IsLinesGeom}}({{TYPE}} *this)",
	"bhkNiCollisionObject *{{IsBhkNiCollisionObject}}({{TYPE}} *this)",
	"bhkBlendCollisionObject *{{IsBhkBlendCollisionObject}}({{TYPE}} *this)",
	"bhkAttachmentCollisionObject *{{IsBhkAttachmentCollisionObject}}({{TYPE}} *this)",
	"bhkRigidBody *{{IsBhkRigidBody}}({{TYPE}} *this)",
	"bhkLimitedHingeConstraint *{{IsBhkLimitedHingeConstraint}}({{TYPE}} *this)",

	"NiObject *{{CreateClone}}({{TYPE}} *this, NiCloningProcess *Process)",
	"void {{LoadBinary}}({{TYPE}} *this, NiStream *)",
	"void {{LinkObject}}({{TYPE}} *this, NiStream *)",
	"bool {{RegisterStreamables}}({{TYPE}} *this, NiStream *Stream)",
	"void {{SaveBinary}}({{TYPE}} *this, NiStream *Stream)",
	"bool {{IsEqual}}({{TYPE}} *this, NiObject *Other)",
	"void {{GetViewerStrings}}({{TYPE}} *this, NiTPrimitiveArray<char const *> *Strings)", // CREATION KIT ONLY
	"void {{ProcessClone}}({{TYPE}} *this, NiCloningProcess *Process)",
	"void {{PostLinkObject}}({{TYPE}} *this, NiStream *Stream)",
	"bool {{StreamCanSkip}}({{TYPE}} *this)",
	"const NiRTTI *{{GetStreamableRTTI}}({{TYPE}} *this)",
	"unsigned int {{GetBlockAllocationSize}}({{TYPE}} *this)",
	"NiObjectGroup *{{GetGroup}}({{TYPE}} *this)",
	"void *{{SetGroup}}({{TYPE}} *this, NiObjectGroup *Group)",
	"NiControllerManager *{{IsNiControllerManager}}({{TYPE}} *this)",
	"void {{AddViewerStrings}}({{TYPE}} *this, NiTPrimitiveArray<char const *> *Strings)", // CREATION KIT ONLY
	nullptr,
};

const int NiObjectCK[] =
{
	27,
	36,
	-1,
};

// NiObjectNET
const char *NiObjectNETFunctions[] =
{
	nullptr,
};

const int NiObjectNETCK[] =
{
	-1,
};

// NiAVObject
const char *NiAVObjectFunctions[] =
{
	"void {{UpdateControllers}}({{TYPE}} *this)",
	"void {{PerformOp}}({{TYPE}} *this)",
	"void {{Unk0}}({{TYPE}} *this)",
	"void {{Unk1}}({{TYPE}} *this)",
	"void {{Unk2}}({{TYPE}} *this)",
	"void {{Unk3}}({{TYPE}} *this)",
	"void {{SetSelectiveUpdateFlags}}({{TYPE}} *this)",
	"void {{UpdateDownwardPass}}({{TYPE}} *this)",
	"void {{UpdateSelectedDownwardPass}}({{TYPE}} *this)",
	"void {{UpdateRigidDownwardPass}}({{TYPE}} *this)",
	"void {{UpdateWorldBound}}({{TYPE}} *this)",
	"void {{UpdateWorldData}}({{TYPE}} *this)",
	"void {{Unk4}}({{TYPE}} *this)",
	"void {{Unk5}}({{TYPE}} *this)",
	"void {{Unk6}}({{TYPE}} *this)",
	"void {{OnVisible}}({{TYPE}} *this, NiCullingProcess *Process, uint32_t Unknown)",
	nullptr,
};

const int NiAVObjectCK[] =
{
	-1,
};

// bhkSerializable
const char *bhkSerializableFunctions[] =
{
	"void {{Unk7}}({{TYPE}} *this)",
	"void {{Unk8}}({{TYPE}} *this)",
	"void {{Unk9}}({{TYPE}} *this)",
	"void {{Unk10}}({{TYPE}} *this)",
	"void {{Unk11}}({{TYPE}} *this)",
	"void {{Unk12}}({{TYPE}} *this)",
	"void {{KillCinfo}}({{TYPE}} *this, bool Created)",
	"unsigned int {{QCinfoSize}}({{TYPE}} *this)",
	"int {{LoadCInfo}}({{TYPE}} *this, NiStream *Stream)",
	"void {{Init}}({{TYPE}} *this, const hkSerializableCinfo *Info)",
	"hkSerializableCinfo *{{CreateCinfo}}({{TYPE}} *this, bool *Created)",
	"void {{PostInit1}}({{TYPE}} *this)",
	"void {{PostInit2}}({{TYPE}} *this)",
	nullptr,
};

const int bhkSerializableCK[] =
{
	-1,
};

void AddTypes(std::vector<const char *>& Vector, const char **Functions, const int *CKFunctions)
{
	for (int i = 0; Functions[i]; i++)
	{
		bool allowInclude = true;

		// Skip any function indexes marked as CretionKit-only
		if (!g_IsCreationKit)
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