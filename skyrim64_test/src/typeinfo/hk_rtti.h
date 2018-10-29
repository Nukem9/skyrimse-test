#pragma once

#include <stdint.h>
#include "../common.h"

namespace HKRTTI
{
#define HK_NULL nullptr
#define HK_CALL	__cdecl					// Redundant on X64

	typedef bool hkBool;				// True 1, False 0

	typedef float  hkReal;				// Default floating point type
	typedef float  hkFloat32;			// Explicitly required float
	typedef double hkDouble64;			// Explicitly required double

	typedef signed char		hkChar;		// Signed 8 bit integer
	typedef signed char		hkInt8;		// Signed 8 bit integer
	typedef signed short	hkInt16;	// Signed 16 bit integer
	typedef signed int		hkInt32;	// Signed 32 bit integer
	typedef signed long long hkInt64;	// Signed 64 bit integer
	typedef unsigned char	hkUchar;	// Unsigned 8 bit integer
	typedef unsigned char	hkUint8;	// Unsigned 8 bit integer
	typedef unsigned short	hkUint16;	// Unsigned 16 bit integer
	typedef unsigned int	hkUint32;	// Unsigned 32 bit integer
	typedef unsigned long long hkUint64;// Unsigned 64 bit integer

	typedef unsigned __int64 hkUlong;	// Pointer-sized type
	typedef signed __int64 hkLong;		// Pointer-sized type

	typedef size_t hk_size_t;

	static_assert(sizeof(hkBool) == 1, "Havok SDK expects bool size to be 1");
	static_assert(sizeof(hkUlong) == sizeof(void *), "Havok SDK expects pointer size to match");

	class hkTypeInfo;
	class hkClass;
	class hkClassEnum;
	class hkClassMember;
	class hkCustomAttributes;

	const char *MemberToString(const hkClassMember *Member);
	void DumpClass(const char *BasePath, const hkClass *Class);
	void DumpReflectionData(const char *BasePath);

	class hkTypeInfo
	{
	public:
		typedef void (HK_CALL *FinishLoadedObjectFunction)(void*, int);
		typedef void (HK_CALL *CleanupLoadedObjectFunction)(void*);

		const char *m_typeName;		// MyClass1MyClass2MyX...
		const char *m_scopedName;	// !MyClass1::MyClass2::MyX...
		FinishLoadedObjectFunction m_finishLoadedObjectFunction;
		CleanupLoadedObjectFunction m_cleanupLoadedObjectFunction;
		const void *m_vtable;		// Non-null if present
		hk_size_t m_size;			// Class size

		static std::vector<hkTypeInfo *> GetGlobalArray();
	};

	class hkClassMember
	{
	public:
		enum Type : hkUint8
		{
			TYPE_VOID = 0,
			TYPE_BOOL = 1,
			TYPE_CHAR = 2,
			TYPE_INT8 = 3,
			TYPE_UINT8 = 4,
			TYPE_INT16 = 5,
			TYPE_UINT16 = 6,
			TYPE_INT32 = 7,
			TYPE_UINT32 = 8,
			TYPE_INT64 = 9,
			TYPE_UINT64 = 10,
			TYPE_REAL = 11,
			TYPE_VECTOR4 = 12,
			TYPE_QUATERNION = 13,
			TYPE_MATRIX3 = 14,
			TYPE_ROTATION = 15,
			TYPE_QSTRANSFORM = 16,
			TYPE_MATRIX4 = 17,
			TYPE_TRANSFORM = 18,
			TYPE_ZERO = 19,
			TYPE_POINTER = 20,
			TYPE_FUNCTIONPOINTER = 21,
			TYPE_ARRAY = 22,
			TYPE_INPLACEARRAY = 23,
			TYPE_ENUM = 24,
			TYPE_STRUCT = 25,
			TYPE_SIMPLEARRAY = 26,
			TYPE_HOMOGENEOUSARRAY = 27,
			TYPE_VARIANT = 28,
			TYPE_CSTRING = 29,
			TYPE_ULONG = 30,
			TYPE_FLAGS = 31,
			TYPE_HALF = 32,
			TYPE_STRINGPTR = 33,
			TYPE_RELARRAY = 34,
			TYPE_MAX = 35,
		};

		enum FlagValues : hkUint16
		{
			FLAGS_NONE = 0,
			ALIGN_8 = 128,
			ALIGN_16 = 256,
			NOT_OWNED = 512,
			SERIALIZE_IGNORED = 1024
		};

		enum DeprecatedFlagValues
		{
			DEPRECATED_SIZE_8 = 8,
			DEPRECATED_ENUM_8 = 8,
			DEPRECATED_SIZE_16 = 16,
			DEPRECATED_ENUM_16 = 16,
			DEPRECATED_SIZE_32 = 32,
			DEPRECATED_ENUM_32 = 32
		};

		const char *m_name;
		const hkClass *m_class;
		const hkClassEnum *m_enum;
		Type m_type;
		Type m_subtype;
		hkInt16 m_cArraySize;
		FlagValues m_flags;
		hkUint16 m_offset;
		const hkCustomAttributes *m_attributes;

		const char *GetName() const
		{
			return m_name;
		}

		Type GetType() const
		{
			return m_type;
		}
	};

	class hkClassEnumItem
	{
	public:
		hkInt32 m_value;
		const char *m_name;

		hkInt32 GetValue() const
		{
			return m_value;
		}

		const char *GetName() const
		{
			return m_name;
		}
	};

	class hkClassEnum
	{
	public:
		enum FlagValues
		{
			FLAGS_NONE = 0,
		};

		const char *m_name;
		const hkClassEnumItem *m_items;
		hkInt32 m_numItems;								// Not serialized
		hkCustomAttributes *m_attributes;
		FlagValues m_flags;

		const char *GetName() const
		{
			return m_name;
		}

		const hkClassEnumItem *GetItem(hkInt32 i) const
		{
			return &m_items[i];
		}

		hkInt32 GetNumItems() const
		{
			return m_numItems;
		}
	};

	class hkClass
	{
	public:
		enum SignatureFlags
		{
			SIGNATURE_LOCAL = 1,
		};

		enum FlagValues
		{
			FLAGS_NONE = 0,
			FLAGS_NOT_SERIALIZABLE = 1,
		};

		const char *m_name;
		const hkClass *m_parent;
		hkInt32 m_objectSize;
		//const hkClass **m_implementedInterfaces;		// Not serialized or defined
		hkInt32 m_numImplementedInterfaces;
		const hkClassEnum *m_declaredEnums;
		hkInt32 m_numDeclaredEnums;						// Not serialized
		const hkClassMember *m_declaredMembers;
		hkInt32 m_numDeclaredMembers;					// Not serialized
		const void *m_defaults;
		const hkCustomAttributes *m_attributes;
		FlagValues m_flags;
		hkInt32 m_describedVersion;

		static std::vector<hkClass *> GetGlobalArray();

		const char *GetName() const
		{
			return m_name;
		}

		const hkClass *GetParent() const
		{
			return m_parent;
		}

		hkInt32 GetNumDeclaredEnums() const
		{
			return m_numDeclaredEnums;
		}

		hkInt32 GetNumDeclaredMembers() const
		{
			return m_numDeclaredMembers;
		}

		const hkClassEnum *GetDeclaredEnum(hkInt32 i) const
		{
			return &m_declaredEnums[i];
		}

		const hkClassMember *GetDeclaredMember(hkInt32 i) const
		{
			return &m_declaredMembers[i];
		}

		bool HasVtable() const
		{
			const hkClass *parent = this;

			for (const hkClass *i = GetParent(); i; i = i->GetParent())
				parent = i;

			return parent->m_numImplementedInterfaces != 0;
		}
	};

	class hkCustomAttributes
	{
	public:
	};
}