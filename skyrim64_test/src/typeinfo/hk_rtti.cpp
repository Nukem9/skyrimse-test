#include <map>
#include "../common.h"
#include "hk_rtti.h"

namespace HKRTTI
{
	const std::map<hkClassMember::Type, const char *> HavokTypeMap
	{
		{ hkClassMember::TYPE_VOID, "void" },
		{ hkClassMember::TYPE_BOOL, "hkBool" },
		{ hkClassMember::TYPE_CHAR, "hkChar" },
		{ hkClassMember::TYPE_INT8, "hkInt8" },
		{ hkClassMember::TYPE_UINT8, "hkUint8" },
		{ hkClassMember::TYPE_INT16, "hkInt16" },
		{ hkClassMember::TYPE_UINT16, "hkUint16" },
		{ hkClassMember::TYPE_INT32, "hkInt32" },
		{ hkClassMember::TYPE_UINT32, "hkUint32" },
		{ hkClassMember::TYPE_INT64, "hkInt64" },
		{ hkClassMember::TYPE_UINT64, "hkUint64" },
		{ hkClassMember::TYPE_REAL, "hkReal" },
		{ hkClassMember::TYPE_VECTOR4, "hkVector4" },
		{ hkClassMember::TYPE_QUATERNION, "hkQuaternion" },
		{ hkClassMember::TYPE_MATRIX3, "hkMatrix3" },
		{ hkClassMember::TYPE_ROTATION, "hkRotation" },
		{ hkClassMember::TYPE_QSTRANSFORM, "hkQsTransform" },
		{ hkClassMember::TYPE_MATRIX4, "hkMatrix4" },
		{ hkClassMember::TYPE_TRANSFORM, "hkTransform" },
		{ hkClassMember::TYPE_ZERO, "hkZero" },
		{ hkClassMember::TYPE_POINTER, nullptr },
		{ hkClassMember::TYPE_FUNCTIONPOINTER, nullptr },
		{ hkClassMember::TYPE_ARRAY, nullptr },
		{ hkClassMember::TYPE_INPLACEARRAY, nullptr },
		{ hkClassMember::TYPE_ENUM, nullptr },
		{ hkClassMember::TYPE_STRUCT, nullptr },
		{ hkClassMember::TYPE_SIMPLEARRAY, nullptr },
		{ hkClassMember::TYPE_HOMOGENEOUSARRAY, nullptr },
		{ hkClassMember::TYPE_VARIANT, "hkRefVariant" },
		{ hkClassMember::TYPE_CSTRING, "char*" },
		{ hkClassMember::TYPE_ULONG, "hkUlong" },
		{ hkClassMember::TYPE_FLAGS, "hkFlags" },
		{ hkClassMember::TYPE_HALF, "hkHalf" },
		{ hkClassMember::TYPE_STRINGPTR, "hkStringPtr" },
		{ hkClassMember::TYPE_RELARRAY, nullptr }
	};

	std::vector<hkTypeInfo *> hkTypeInfo::GetGlobalArray()
	{
		uintptr_t base = (uintptr_t)GetModuleHandleA(nullptr);
		auto typeArray = (hkTypeInfo **)(base + 0x3288960);

		std::vector<hkTypeInfo *> out;

		for (; *typeArray != nullptr; typeArray++)
			out.push_back(*typeArray);

		return out;
	}

	std::vector<hkClass *> hkClass::GetGlobalArray()
	{
		uintptr_t base = (uintptr_t)GetModuleHandleA(nullptr);
		auto classArray = (hkClass **)(base + 0x3289AC0);

		std::vector<hkClass *> out;

		for (; *classArray != nullptr; classArray++)
			out.push_back(*classArray);

		return out;
	}

	const char *MemberToString(const hkClassMember *Member)
	{
		static char buf[512];

		hkClassMember::Type type = Member->m_type;
		const char *baseType = HavokTypeMap.at(type);

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
		else if (HavokTypeMap.at(Member->m_subtype))
		{
			sprintf_s(subType, "%s", HavokTypeMap.at(Member->m_subtype));
		}
		else if (Member->m_subtype == hkClassMember::TYPE_POINTER)
		{
			//sprintf_s(subType, "POINTER");
		}
		else
		{
			__debugbreak();
		}

		if (Member->m_subtype == hkClassMember::TYPE_POINTER)
			strcat_s(subType, " *");

		switch (type)
		{
			buf[0] = '\0';

		case hkClassMember::TYPE_POINTER:
			sprintf_s(buf, "%s *m_%s", subType, Member->m_name);
			break;

		case hkClassMember::TYPE_FUNCTIONPOINTER:
			sprintf_s(buf, "FUNCPTR<%s> m_%s", subType, Member->m_name);
			break;

		case hkClassMember::TYPE_ARRAY:
			sprintf_s(buf, "hkArray<%s> m_%s", subType, Member->m_name);
			break;

		case hkClassMember::TYPE_INPLACEARRAY:
			sprintf_s(buf, "<INPLACEARRAY> m_%s", Member->m_name);
			break;

		case hkClassMember::TYPE_ENUM:
			sprintf_s(buf, "%s m_%s", subType, Member->m_name);
			break;

		case hkClassMember::TYPE_STRUCT:
			sprintf_s(buf, "%s m_%s", subType, Member->m_name);
			break;

		case hkClassMember::TYPE_SIMPLEARRAY:
			sprintf_s(buf, "const %s *m_%s", subType, Member->m_name);
			break;

		case hkClassMember::TYPE_HOMOGENEOUSARRAY:
			sprintf_s(buf, "<HOMOGENEOUSARRAY> m_%s", Member->m_name);
			break;

		case hkClassMember::TYPE_VARIANT:
			sprintf_s(buf, "hkVariant m_%s", Member->m_name);
			break;

		case hkClassMember::TYPE_FLAGS:
			sprintf_s(buf, "hkFlags<%s, %s> m_%s", subType, HavokTypeMap.at(Member->m_subtype), Member->m_name);
			break;

		case hkClassMember::TYPE_RELARRAY:
			sprintf_s(buf, "<RELARRAY> m_%s", Member->m_name);
			break;

		default:
			__debugbreak();
			break;
		}

		return buf;
	}

	void DumpClass(const char *BasePath, const hkClass *Class)
	{
		const char *className = Class->m_name;
		const hkClass *classParent = Class->GetParent();

		// Create a specific header file for each class
		char filePath[1024];
		sprintf_s(filePath, "%s\\%s.h", BasePath, Class->m_name);

		FILE *f = nullptr;
		if (fopen_s(&f, filePath, "w") != 0)
			return;

		auto print = [f](const char *Format, ...)
		{
			va_list va;
			va_start(va, Format);

			vfprintf(f, Format, va);
			va_end(va);
		};

		print("#pragma once\n\n");
		print("extern const hkClass %sClass;\n\n", className);
		print("// Class: %s (Version %d)\n", className, Class->m_describedVersion, Class->m_describedVersion);
		print("// Size: %d (0x%X)\n", Class->m_objectSize, Class->m_objectSize);

		if (classParent)
			print("class %s : public %s\n", className, classParent->m_name);
		else
			print("class %s\n", className);

		print("{\n");
		print("public:\n");

		if (Class->m_describedVersion != 0)
			print("\t// +version(%d)\n", Class->m_describedVersion);

		print("\tHK_DECLARE_REFLECTION();\n");

		if (Class->HasVtable())
			print("\tHK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_BEHAVIOR_RUNTIME);\n");
		else
			print("\tHK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_BEHAVIOR_RUNTIME, %s);\n", className);

		// Constructors
		print("\n");
		print("\tHK_FORCE_INLINE %s(void) {}\n", className);

		if (Class->HasVtable())
		{
			if (!classParent)
				print("\tHK_FORCE_INLINE %s(hkFinishLoadedObjectFlag flag)", className);
			else
				print("\tHK_FORCE_INLINE %s(hkFinishLoadedObjectFlag flag) : %s(flag)", className, classParent->GetName());

			if (Class->GetNumDeclaredMembers() > 0)
				print("\n");

			bool first = classParent == nullptr;

			for (int i = 0; i < Class->GetNumDeclaredMembers(); i++)
			{
				const hkClassMember *member = Class->GetDeclaredMember(i);

				switch (member->GetType())
				{
				case hkClassMember::TYPE_POINTER:
				case hkClassMember::TYPE_ARRAY:
				case hkClassMember::TYPE_STRINGPTR:
				case hkClassMember::TYPE_VARIANT:
					if (first)
						print("\t\tm_%s(flag)\n", member->GetName());
					else
						print("\t\t, m_%s(flag)\n", member->GetName());

					first = false;
					break;
				}
			}

			if (Class->GetNumDeclaredMembers() > 0)
				print("\t\t{}\n");
			else
				print(" {}\n");
		}

		// Enums/flags
		if (Class->GetNumDeclaredEnums() > 0)
		{
			print("\n");

			for (int i = 0; i < Class->GetNumDeclaredEnums(); i++)
			{
				const hkClassEnum *classEnum = Class->GetDeclaredEnum(i);

				print("\tenum %s\n", classEnum->GetName());
				print("\t{\n");

				for (int j = 0; j < classEnum->GetNumItems(); j++)
				{
					const hkClassEnumItem *item = classEnum->GetItem(j);

					print("\t\t%s = %d,\n", item->GetName(), item->GetValue());
				}

				print("\t};\n");
			}
		}

		// Properties
		if (Class->GetNumDeclaredMembers() > 0)
		{
			print("\n\t// Properties\n");

			for (int i = 0; i < Class->GetNumDeclaredMembers(); i++)
			{
				const hkClassMember *member = Class->GetDeclaredMember(i);

				print("\t%s;\n", MemberToString(member));
			}
		}

		print("};\n\n");
		fclose(f);
	}

	void DumpReflectionData(const char *BasePath)
	{
		auto typeinfo = hkTypeInfo::GetGlobalArray();
		auto classes = hkClass::GetGlobalArray();

		// Add all parent classes ONCE
		std::unordered_map<const hkClass *, bool> classMap;

		for (const hkClass *item : classes)
		{
			for (const hkClass *i = item; i; i = i->GetParent())
				classMap.insert_or_assign(i, true);
		}

		// Enumerate everything
		for (const auto& item : classMap)
			DumpClass(BasePath, item.first);
	}
}