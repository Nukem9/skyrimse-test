#include "../stdafx.h"
#include "hk_typeinfo.h"
#include <map>

std::map<hk_ClassMember::Type, const char *> HavokTypeMap
{
	{ hk_ClassMember::TYPE_VOID, "void" },
	{ hk_ClassMember::TYPE_BOOL, "hkBool" },
	{ hk_ClassMember::TYPE_CHAR, "hkChar" },
	{ hk_ClassMember::TYPE_INT8, "hkInt8" },
	{ hk_ClassMember::TYPE_UINT8, "hkUint8" },
	{ hk_ClassMember::TYPE_INT16, "hkInt16" },
	{ hk_ClassMember::TYPE_UINT16, "hkUint16" },
	{ hk_ClassMember::TYPE_INT32, "hkInt32" },
	{ hk_ClassMember::TYPE_UINT32, "hkUint32" },
	{ hk_ClassMember::TYPE_INT64, "hkInt64" },
	{ hk_ClassMember::TYPE_UINT64, "hkUint64" },
	{ hk_ClassMember::TYPE_REAL, "hkReal" },
	{ hk_ClassMember::TYPE_VECTOR4, "hkVector4" },
	{ hk_ClassMember::TYPE_QUATERNION, "hkQuaternion" },
	{ hk_ClassMember::TYPE_MATRIX3, "hkMatrix3" },
	{ hk_ClassMember::TYPE_ROTATION, "hkRotation" },
	{ hk_ClassMember::TYPE_QSTRANSFORM, "hkQsTransform" },
	{ hk_ClassMember::TYPE_MATRIX4, "hkMatrix4" },
	{ hk_ClassMember::TYPE_TRANSFORM, "hkTransform" },
	{ hk_ClassMember::TYPE_ZERO, "hkZero" },
	{ hk_ClassMember::TYPE_POINTER, nullptr },
	{ hk_ClassMember::TYPE_FUNCTIONPOINTER, nullptr },
	{ hk_ClassMember::TYPE_ARRAY, nullptr },
	{ hk_ClassMember::TYPE_INPLACEARRAY, nullptr },
	{ hk_ClassMember::TYPE_ENUM, nullptr },
	{ hk_ClassMember::TYPE_STRUCT, nullptr },
	{ hk_ClassMember::TYPE_SIMPLEARRAY, nullptr },
	{ hk_ClassMember::TYPE_HOMOGENEOUSARRAY, nullptr },
	{ hk_ClassMember::TYPE_VARIANT, nullptr },
	{ hk_ClassMember::TYPE_CSTRING, "char*" },
	{ hk_ClassMember::TYPE_ULONG, "hkUlong" },
	{ hk_ClassMember::TYPE_FLAGS, nullptr },
	{ hk_ClassMember::TYPE_HALF, "hkHalf" },
	{ hk_ClassMember::TYPE_STRINGPTR, "hkStringPtr" },
	{ hk_ClassMember::TYPE_RELARRAY, nullptr }
};

std::vector<hk_TypeInfo *> hk_TypeInfo::GetGlobalArray()
{
	uintptr_t base = (uintptr_t)GetModuleHandleA(nullptr);
	auto typeArray = (hk_TypeInfo **)(base + 0x17B3E50);

	std::vector<hk_TypeInfo *> out;

	for (; *typeArray != nullptr; typeArray++)
		out.push_back(*typeArray);

	return out;
}

std::vector<hk_Class *> hk_Class::GetGlobalArray()
{
	uintptr_t base = (uintptr_t)GetModuleHandleA(nullptr);
	auto classArray = (hk_Class **)(base + 0x17B4F80);

	std::vector<hk_Class *> out;

	for (; *classArray != nullptr; classArray++)
		out.push_back(*classArray);

	return out;
}

const char *MemberToString(const hk_ClassMember *Member)
{
	static char buf[512];

	hk_ClassMember::Type type = Member->m_type;
	const char *baseType = HavokTypeMap[type];

	// Take the easy way out if it was a simple type
	if (baseType)
	{
		if (Member->m_cArraySize == 0)
			sprintf_s(buf, "%s m_%s", baseType, Member->m_name);
		else
			sprintf_s(buf, "%s m_%s[%d]", baseType, Member->m_name, (hkInt32)Member->m_cArraySize);

		return buf;
	}

	// Determine subtype first if it's present
	char subType[512];
	sprintf_s(subType, "UNKNOWN");

	if (Member->m_class)
	{
		sprintf_s(subType, "%s", Member->m_class->m_name);
	}
	else if (Member->m_enum)
	{
		sprintf_s(subType, "%s", Member->m_enum->m_name);
	}
	else if (HavokTypeMap[Member->m_subtype])
	{
		sprintf_s(subType, "%s", HavokTypeMap[Member->m_subtype]);
	}
	else if (Member->m_subtype == hk_ClassMember::TYPE_POINTER)
	{
		//sprintf_s(subType, "POINTER");
	}
	else
	{
		__debugbreak();
	}

	if (Member->m_subtype == hk_ClassMember::TYPE_POINTER)
		strcat_s(subType, "*");

	switch (type)
	{
		buf[0] = '\0';

		case hk_ClassMember::TYPE_POINTER:
			sprintf_s(buf, "%s* m_%s", subType, Member->m_name);
			break;

		case hk_ClassMember::TYPE_FUNCTIONPOINTER:
			sprintf_s(buf, "FUNCPTR<%s> m_%s", subType, Member->m_name);
			break;

		case hk_ClassMember::TYPE_ARRAY:
			sprintf_s(buf, "hkArray<%s> m_%s", subType, Member->m_name);
			break;

		case hk_ClassMember::TYPE_INPLACEARRAY:
			sprintf_s(buf, "<INPLACEARRAY> m_%s", Member->m_name);
			break;

		case hk_ClassMember::TYPE_ENUM:
			sprintf_s(buf, "%s m_%s", subType, Member->m_name);
			break;

		case hk_ClassMember::TYPE_STRUCT:
			sprintf_s(buf, "%s m_%s", subType, Member->m_name);
			break;

		case hk_ClassMember::TYPE_SIMPLEARRAY:
			sprintf_s(buf, "const %s* m_%s", subType, Member->m_name);
			break;

		case hk_ClassMember::TYPE_HOMOGENEOUSARRAY:
			sprintf_s(buf, "<HOMOGENEOUSARRAY> m_%s", Member->m_name);
			break;

		case hk_ClassMember::TYPE_VARIANT:
			sprintf_s(buf, "hkVariant m_%s", Member->m_name);
			break;

		case hk_ClassMember::TYPE_FLAGS:
			sprintf_s(buf, "hkFlags<%s, %s> m_%s", subType, HavokTypeMap[Member->m_subtype], Member->m_name);
			break;

		case hk_ClassMember::TYPE_RELARRAY:
			sprintf_s(buf, "<RELARRAY> m_%s", Member->m_name);
			break;

		default:
			__debugbreak();
			break;
	}

	return buf;
}

void DumpEnum(const hk_ClassEnum *Enum)
{
	printf("enum %s\n{\n", Enum->m_name);

	for (int i = 0; i < Enum->m_numItems; i++)
		printf("%s = %d,\n", Enum->m_items[i].m_name, Enum->m_items[i].m_value);

	printf("};\n\n");
}

void DumpClass(hk_Class *Class)
{
	if (Class->m_parent)
		printf("\n\n// (%llX)\nclass %s : public %s\n{\n", Class, Class->m_name, Class->m_parent->m_name);
	else
		printf("\n\n// (%llX)\nclass %s\n{\n", Class, Class->m_name);

	// Iterate over each enum in the class
	for (int i = 0; i < Class->m_numDeclaredEnums; i++)
		DumpEnum(&Class->m_declaredEnums[i]);

	// Iterate over each member
	for (int i = 0; i < Class->m_numDeclaredMembers; i++)
	{
		auto member = &Class->m_declaredMembers[i];

		if (!member)
			__debugbreak();

		printf("\t%s;// @ 0x%04X\n", MemberToString(member), member->m_offset);
	}

	printf("};\n");
	printf("static_assert(sizeof(%s) == 0x%X, \"Invalid class size\");\n", Class->m_name, Class->m_objectSize);

	// Now dump all subclasses of this parent
	auto classes = hk_Class::GetGlobalArray();

	for (auto& subClass : classes)
	{
		if (subClass->m_parent == Class)
			DumpClass(subClass);
	}
}

void DumpReflectionData()
{
	freopen("C:\\out.txt", "w", stdout);

	auto typeinfo = hk_TypeInfo::GetGlobalArray();
	auto classes = hk_Class::GetGlobalArray();

	// Enumerate each top-level class first
	for (const auto& item : classes)
	{
		if (item->m_parent)
			continue;

		DumpClass(item);
	}

	getchar();
	fflush(stdout);
}