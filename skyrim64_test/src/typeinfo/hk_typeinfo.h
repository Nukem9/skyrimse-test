#pragma once

#include <stdint.h>
#include <vector>

#define HK_NULL nullptr
#define HK_CALL	__cdecl				// Redundant on X64

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

struct hk_TypeInfo;
struct hk_Class;
struct hk_ClassMember;
struct hk_ClassEnum;
struct hk_CustomAttributes;

struct hk_TypeInfo
{
	typedef void (HK_CALL *FinishLoadedObjectFunction)(void*, int);
	typedef void (HK_CALL *CleanupLoadedObjectFunction)(void*);

	const char* m_typeName;		// MyClass1MyClass2MyX...
	const char* m_scopedName;	// !MyClass1::MyClass2::MyX...
	FinishLoadedObjectFunction m_finishLoadedObjectFunction;
	CleanupLoadedObjectFunction m_cleanupLoadedObjectFunction;
	const void* m_vtable;		// Non-null if present
	hk_size_t m_size;			// Class size

	static std::vector<hk_TypeInfo *> GetGlobalArray();
};

struct hk_Class
{
	enum Flags : hkUint32
	{
		FLAGS_NONE = 0,
		FLAGS_NOT_SERIALIZABLE = 1
	};

	const char* m_name;
	const hk_Class* m_parent;
	int m_objectSize;
	//const hk_Class** m_implementedInterfaces;
	int m_numImplementedInterfaces;
	const struct hk_ClassEnum* m_declaredEnums;
	int m_numDeclaredEnums;
	const struct hk_ClassMember* m_declaredMembers;
	int m_numDeclaredMembers;
	const void* m_defaults;
	const hk_CustomAttributes* m_attributes;
	Flags m_flags;
	int m_describedVersion;

	static std::vector<hk_Class *> GetGlobalArray();
};

struct hk_ClassMember
{
	enum Type : hkUint8
	{
		TYPE_VOID = 0,
		TYPE_BOOL,
		TYPE_CHAR,
		TYPE_INT8,
		TYPE_UINT8,
		TYPE_INT16,
		TYPE_UINT16,
		TYPE_INT32,
		TYPE_UINT32,
		TYPE_INT64,
		TYPE_UINT64,
		TYPE_REAL,
		TYPE_VECTOR4,
		TYPE_QUATERNION,
		TYPE_MATRIX3,
		TYPE_ROTATION,
		TYPE_QSTRANSFORM,
		TYPE_MATRIX4,
		TYPE_TRANSFORM,
		TYPE_ZERO,
		TYPE_POINTER,
		TYPE_FUNCTIONPOINTER,
		TYPE_ARRAY,
		TYPE_INPLACEARRAY,
		TYPE_ENUM,
		TYPE_STRUCT,
		TYPE_SIMPLEARRAY,
		TYPE_HOMOGENEOUSARRAY,
		TYPE_VARIANT,
		TYPE_CSTRING,
		TYPE_ULONG,
		TYPE_FLAGS,
		TYPE_HALF,
		TYPE_STRINGPTR,
		TYPE_RELARRAY,
		TYPE_MAX
	};

	enum Flags : hkUint16
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

	enum
	{
		HK_CLASS_ZERO_DEFAULT = -2,
	};

	const char* m_name;
	const hk_Class* m_class;
	const hk_ClassEnum* m_enum;
	Type m_type;
	Type m_subtype;
	hkInt16 m_cArraySize;
	Flags m_flags;
	hkUint16 m_offset;
	const hk_CustomAttributes* m_attributes;
};

struct hk_ClassEnum
{
	struct Item
	{
		int m_value;
		const char* m_name;
	};

	enum Flags : hkUint32
	{
		FLAGS_NONE = 0
	};

	const char* m_name;
	const struct Item* m_items;
	int m_numItems;
	hk_CustomAttributes* m_attributes;
	Flags m_flags;
};

struct hk_CustomAttributes
{
};