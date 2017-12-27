#pragma once

#include <vector>
#include <type_traits>
#pragma warning(disable:4094) // untagged 'struct' declared no symbols

#define VTABLE_FUNCTION_INDEX(Function) vtable_index_util::getIndexOf(&Function)

class vtable_index_util
{
private:
	typedef int(*VtableIndexFn)();
	static vtable_index_util *GlobalInstance;

public:
	static vtable_index_util *Instance();

	template<typename T>
	static int getIndexOf(T ptr)
	{
		return (Instance()->**((decltype(&ForceVtableReference)*)(&ptr)))();
	}

private:
	virtual int ForceVtableReference();
};

template<void(*ctor)()>
struct static_constructor
{
	struct constructor { constructor() { ctor(); } };
	static constructor c;
};

template<void(*ctor)()>
typename static_constructor<ctor>::constructor static_constructor<ctor>::c;

#pragma pack(push, 8)  
const DWORD MS_VC_EXCEPTION = 0x406D1388;

typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;		// Must be 0x1000.  
	LPCSTR szName;		// Pointer to name (in user addr space).  
	DWORD dwThreadID;	// Thread ID (-1=caller thread).  
	DWORD dwFlags;		// Reserved for future use, must be zero.  
} THREADNAME_INFO;
#pragma pack(pop)

#define STATIC_CONSTRUCTOR(Id, Lambda) struct { static void Id(){ static_constructor<&Id>::c; Lambda(); } };

#define DECLARE_CONSTRUCTOR_HOOK(Class) \
	static Class *__ctor__(void *Instance) \
	{ \
		return new (Instance) Class(); \
	} \
	\
	static Class *__dtor__(Class *Thisptr, unsigned __int8) \
	{ \
		Thisptr->~Class(); \
		return Thisptr; \
	}

intptr_t FindPattern(const std::vector<unsigned char>& data, intptr_t baseAddress, const unsigned char *lpPattern, const char *pszMask, intptr_t offset, intptr_t resultUsage);
uintptr_t FindPatternSimple(uintptr_t StartAddress, uintptr_t MaxSize, const BYTE *ByteMask, const char *Mask);
void PatchMemory(ULONG_PTR Address, PBYTE Data, SIZE_T Size);
void SetThreadName(DWORD dwThreadID, const char *ThreadName);
void Trim(char *Buffer, char C);

#define templated(...) __VA_ARGS__
#define AutoPtr(Type, Name, Offset) static Type& Name = (*(Type *)((uintptr_t)GetModuleHandle(nullptr) + Offset))

#define static_assert_offset(Structure, Member, Offset) { CheckOffset<offsetof(Structure, Member), Offset> templated(__z)__COUNTER__; }
#define assert_vtable_index(Function, Index) assert(vtable_index_util::getIndexOf(Function) == Index)

template <size_t Offset, size_t RequiredOffset>
struct CheckOffset
{
	static_assert(Offset <= RequiredOffset, "Offset is larger than expected");
	static_assert(Offset >= RequiredOffset, "Offset is smaller than expected");
};