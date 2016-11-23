#include "stdafx.h"
#include <typeinfo>
#include <unordered_map>

std::vector<ULONG_PTR> RTTIPointers;
std::unordered_map<ULONG_PTR, ULONG_PTR> RTTIPointerReferenceMap;
std::vector<type_info *> TypeInfo;

class InheritanceInfo
{
public:
	enum class Type
	{
		Public,
		Protected,
		Private,
	};

	bool IsVirtual		= false;
	Type Type			= Type::Public;
	type_info *RTTIType = nullptr;
};

class ParsedTypeInfo
{
public:
	char Name[512];
	bool IsStruct   : 1;
	bool IsClass    : 1;
	bool IsTemplate : 1;
	bool IsUndefined: 1;

	std::vector<InheritanceInfo> Inherits;
};

class TypeNamespace
{
public:
	char Name[512];
	std::vector<TypeNamespace> *SubSpaces;
	std::vector<ParsedTypeInfo> *Types;
};

TypeNamespace globalNamespace;

struct RTTITypeDescriptor
{
	void** pVMT;
	DWORD spare;
	char name[1];
};

struct RTTIBaseClassArray
{
	DWORD arrayOfBaseClassDescriptors[]; // RTTIBaseClassDescriptor *
};

enum
{
	COL_Signature32 = 0,
	COL_Signature64 = 1,
};

struct RTTICompleteObjectLocator
{
	DWORD signature;		// 32-bit zero, 64-bit one, until loaded
	DWORD offset;			// Offset of this vtable in the complete class
	DWORD cdOffset;			// Constructor displacement offset
	DWORD typeDescriptor;	// TypeDescriptor of the complete class
	DWORD classDescriptor;	// Describes inheritance hierarchy
};

enum
{
	HCD_NoInheritance			= 0,
	HCD_MultipleInheritance		= 1,
	HCD_VirtualInheritance		= 2,
	HCD_AmbiguousInheritance	= 4,
};

struct RTTIClassHierarchyDescriptor
{
	DWORD signature;		// Always zero or one
	DWORD attributes;		// Flags
	DWORD numBaseClasses;	// Number of classes in baseClassArray
	DWORD baseClassArray;	// RTTIBaseClassArray
};

struct PMD
{
	int mdisp;	// Member displacement (vftable offset)
	int pdisp;	// Vbtable displacement (vbtable offset, -1: vftable is at displacement PMD.mdisp inside the class)
	int vdisp;	// Displacement inside vbtable
};

enum
{
	BCD_NotVisible				= 1,
	BCD_Ambiguous				= 2,
	BCD_Private					= 4,
	BCD_PrivOrProtBase			= 8,
	BCD_Virtual					= 16,
	BCD_Nonpolymorphic			= 32,
	BCD_HasHierarchyDescriptor	= 64,
};

struct RTTIBaseClassDescriptor
{
	DWORD typeDescriptor;		// Type descriptor of the class
	DWORD numContainedBases;	// Number of nested classes following in the Base Class Array
	PMD disp;					// Pointer-to-member displacement info
	DWORD attributes;			// Flags
};

bool IsInCode(ULONG_PTR Address)
{
	return (Address >= g_CodeBase && Address < (g_CodeBase + g_CodeSize));
}

bool IsInData(ULONG_PTR Address)
{
	// rdata
	if (Address >= (g_ModuleBase + 0x014B9000) && Address < (g_ModuleBase + 0x1D34000))
		return true;

	// data
	return (Address >= (g_ModuleBase + 0x01D34000) && Address < (g_ModuleBase + 0x3505000));
}

void AddRTTIBase(ULONG_PTR Ptr)
{
	if (IsInData(Ptr))
	{
		// This vtable index must point *back* to code
		if (!IsInCode(*(ULONG_PTR *)Ptr))
			return;

		// (FUNCTION_PTR - 0x8) == RTTI pointer (supposedly)
		ULONG_PTR rttiBase = *(ULONG_PTR *)(Ptr - 0x8);

		// Info can be in .data or .rdata, but not .text
		if (IsInData(rttiBase))
		{
			RTTIPointerReferenceMap[rttiBase] = Ptr;
			RTTIPointers.push_back(rttiBase);
		}
	}
}

void ExtractName(type_info *Type, char *Buffer, size_t Size, char *Scope, size_t ScopeSize)
{
	memset(Buffer, 0, Size);
	memset(Scope, 0, ScopeSize);

	// Skip extra information
	const char *name = Type->name();

	if (_strnicmp(name, "class ", 6) == 0)
		name += 6;
	else if (_strnicmp(name, "struct ", 7) == 0)
		name += 7;

	// '\0' or '<' indicates the name should end.
	// "::" indicates a namespace resolution.
	for (size_t i = 0; name[i] != '\0'; i++)
	{
		// Clear the buffer if we hit ::
		if (name[i] == ':' && name[i + 1] == ':')
		{
			// Update namespace first
			strcat_s(Scope, ScopeSize, Buffer);
			strcat_s(Scope, ScopeSize, "::");

			strcpy_s(Buffer, Size, "");
		}

		// Never include special symbols
		if (name[i] == ':' || name[i] == '*' || name[i] == '>')
			continue;

		// End if we hit <
		if (name[i] == '<')
			break;

		char temp[] = { name[i], '\0' };
		strncat_s(Buffer, Size, temp, 1);
	}
}

FILE *globalFile;

void AddTypeinfo(ULONG_PTR RTTI, type_info *Type)
{
	// class BSScript::NativeFunction5<class TESObjectREFR,bool,class TESObjectREFR * __ptr64,float,float,float,bool>
	// class SkyrimScript::ConcreteDelayFunctorFactory<class SkyrimScript::`anonymous namespace'::MoveToFunctor,0>
	// class CombatThreat
	// struct BSTDerivedCreator<class CombatInventoryItemMagicT<class CombatInventoryItemScroll,class CombatMagicCasterLight>,class CombatObject>

	ParsedTypeInfo info;

	// First determine if struct, class, or something else
	if (_strnicmp(Type->name(), "class ", 6) == 0)
		info.IsClass = true;
	else if (_strnicmp(Type->name(), "struct ", 7) == 0)
		info.IsStruct = true;
	else { }

	char _space[512];
	ExtractName(Type, info.Name, 512, _space, 512);

	// Templates are used when the name lengths don't match up with the original
	info.IsTemplate = false;

	// Resolve inheritance
	auto object		= (RTTICompleteObjectLocator *)RTTI;
	auto hierarchy	= (RTTIClassHierarchyDescriptor *)(g_ModuleBase + object->classDescriptor);

	// fprintf(globalFile, "C%02d V%02d Name: %s%s\r\n", object->cdOffset, object->offset, _space, info.Name);

	if (true)
	{
		auto baseClassArray = (RTTIBaseClassArray *)(g_ModuleBase + hierarchy->baseClassArray);

		// Skip the first index since it's a self-pointer to RTTICompleteObjectLocator
		for (DWORD i = 1; i < hierarchy->numBaseClasses; i++)
		{
			auto baseClassInfo	= (RTTIBaseClassDescriptor *)(g_ModuleBase + baseClassArray->arrayOfBaseClassDescriptors[i]);
			auto type			= (type_info *)(g_ModuleBase + baseClassInfo->typeDescriptor);

			InheritanceInfo baseEntry;
			baseEntry.RTTIType = type;

			if (baseClassInfo->attributes & BCD_Virtual)
				baseEntry.IsVirtual = true;

			if (baseClassInfo->attributes & BCD_Private)
				baseEntry.Type = InheritanceInfo::Type::Private;

			info.Inherits.push_back(baseEntry);

			// Skip the nested base classes (the complete object does not inherit these explicitly, see below)
			i += baseClassInfo->numContainedBases;

			// class TESLevItem : public TESBoundObject, public TESLeveledList
			//
			// C:00 V00 Name: TESLevItem
			//	0040 - M00 P-1 V00 class TESBoundObject
			//	0040 - M00 P-1 V00 class TESObject				// Should not show up in output
			//	0040 - M00 P-1 V00 class TESForm				// ^
			//	0042 - M00 P-1 V00 class BaseFormComponent		// ^
			//	0040 - M48 P-1 V00 class TESLeveledList
			//	0042 - M48 P-1 V00 class BaseFormComponent		// ^
		}
	}

	if (info.IsClass)
		fprintf(globalFile, "class %s%s", _space, info.Name);
	else
		fprintf(globalFile, "struct %s%s", _space, info.Name);

	if (info.Inherits.size() > 0)
	{
		char inheritanceString[4096];
		strcpy_s(inheritanceString, " : ");

		// Append base classes on the same line
		for (auto baseClass : info.Inherits)
		{
			if (baseClass.Type == InheritanceInfo::Type::Private)
				strcat_s(inheritanceString, "private ");
			else
				strcat_s(inheritanceString, "public ");

			if (baseClass.IsVirtual)
				strcat_s(inheritanceString, "virtual ");

			strcat_s(inheritanceString, baseClass.RTTIType->name());
			strcat_s(inheritanceString, ", ");
		}

		// Trim the final comma
		*strrchr(inheritanceString, ',') = '\0';

		fprintf(globalFile, "%s", inheritanceString);
	}

	fprintf(globalFile, "\r\n", _space, info.Name);
	fprintf(globalFile, "{\r\n", _space, info.Name);
	if (info.IsClass)
		fprintf(globalFile, "public:\r\n", _space, info.Name);

	// vtable test (index is guaranteed to exist in the map)
	auto vtableStart = RTTIPointerReferenceMap[RTTI];

	for (int i = 0;; i++)
	{
		ULONG_PTR functionAddr = *(ULONG_PTR *)vtableStart;

		if (!IsInCode(functionAddr))
			break;

		fprintf(globalFile, "\tvirtual void VFunction%03d(); // %llX\r\n", i, functionAddr);
		vtableStart += 8;
	}

	fprintf(globalFile, "};\r\n\r\n", _space, info.Name);
}

void DumpVTables()
{
	if (AllocConsole())
	{
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		freopen("CONIN$", "r", stdin);
	}

	globalFile = fopen("C:\\test.txt", "wb");

	// First scan for all pointers in the .text section that
	// reference the .data/.rdata sections
	for (ULONG_PTR i = g_CodeBase; i < (g_CodeBase + g_CodeSize); i++)
	{
		// Check full 64-bit pointers (rare)
		AddRTTIBase(*(ULONG_PTR *)i);

		// Now check RIP-relative instructions. Ex:
		// .text:00007FF7A265E796 48 8D 05 9B F9 13 01        lea rax, IBSTCreator<BSPathingRequest>::`vftable'
		//       00007FF7A265E796 + 0113F99B + 7 (ins. len)   == IBSTCreator<BSPathingRequest>::`vftable'
		for (int j = 0; j < 7; j++)
		{
			ULONG_PTR ptr = i + *(DWORD *)i + j;
			AddRTTIBase(ptr);
		}
	}

	// Remove duplicate entries
	RTTIPointers.erase(std::unique(RTTIPointers.begin(), RTTIPointers.end()), RTTIPointers.end());

	// Remove invalid entries and duplicated type information
	RTTIPointers.erase(std::remove_if(RTTIPointers.begin(), RTTIPointers.end(), [](ULONG_PTR Elem)
	{
		// Check for the magic value (DWORD RTTICompleteObjectLocator::Signature, 0x1)
		auto object = (RTTICompleteObjectLocator *)Elem;

		if (object->signature != COL_Signature64)
			return false;

		// Check if this type_info was already used
		type_info *ti	= (type_info *)(g_ModuleBase + object->typeDescriptor);
		auto found		= std::find(TypeInfo.begin(), TypeInfo.end(), ti);

		if (found != TypeInfo.end())
		{
			// Entry already exists, skip the duplicate
			return false;
		}

		AddTypeinfo(Elem, ti);
		TypeInfo.push_back(ti);
		return true;
	}), RTTIPointers.end());

	// Initialize the global namespace container, which will hold all parsed type
	// information and child namespaces
	strcpy_s(globalNamespace.Name, "");
	globalNamespace.SubSpaces	= new std::vector<TypeNamespace>();
	globalNamespace.Types		= new std::vector<ParsedTypeInfo>();

	fflush(globalFile);
	fclose(globalFile);

	printf("Found %lld possible VTable references\n", RTTIPointers.size());
	getchar();

}